#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <net/if.h>
#include <time.h>

#include <linux/can.h>
#include <linux/can/raw.h>

int main(int argc, char *argv[])
{
    int socket_fd;
    struct sockaddr_can address;
    struct ifreq interface_request;
    struct can_frame frame;

    int speed = 0;
    int rpm = 0;
    int temperature = 0;

    /*
     * Variables for Vehicle ECU offline detection
     */
    time_t last_speed_time = time(NULL);
    int vehicle_offline = 0;

    /*
     * Create CAN socket
     */
    socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    if (socket_fd < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    /*
     * -----------------------------
     * CAN MESSAGE FILTER
     * -----------------------------
     */

    struct can_filter filter;

    if (argc > 1)
    {
        if (strcmp(argv[1], "speed") == 0)
        {
            filter.can_id = 0x100;
            filter.can_mask = CAN_SFF_MASK;

            if (setsockopt(socket_fd,
                           SOL_CAN_RAW,
                           CAN_RAW_FILTER,
                           &filter,
                           sizeof(filter)) < 0)
            {
                perror("Speed filter failed");
                close(socket_fd);
                return 1;
            }

            printf("Dashboard filter: SPEED only (0x100)\n");
        }

        else if (strcmp(argv[1], "rpm") == 0)
        {
            filter.can_id = 0x101;
            filter.can_mask = CAN_SFF_MASK;

            if (setsockopt(socket_fd,
                           SOL_CAN_RAW,
                           CAN_RAW_FILTER,
                           &filter,
                           sizeof(filter)) < 0)
            {
                perror("RPM filter failed");
                close(socket_fd);
                return 1;
            }

            printf("Dashboard filter: RPM only (0x101)\n");
        }

        else if (strcmp(argv[1], "temperature") == 0)
        {
            filter.can_id = 0x102;
            filter.can_mask = CAN_SFF_MASK;

            if (setsockopt(socket_fd,
                           SOL_CAN_RAW,
                           CAN_RAW_FILTER,
                           &filter,
                           sizeof(filter)) < 0)
            {
                perror("Temperature filter failed");
                close(socket_fd);
                return 1;
            }

            printf("Dashboard filter: TEMPERATURE only (0x102)\n");
        }

        else if (strcmp(argv[1], "all") == 0)
        {
            printf("Dashboard filter: ALL messages\n");
        }

        else
        {
            printf("Unknown filter option.\n");
            printf("Use: all | speed | rpm | temperature\n");

            close(socket_fd);
            return 1;
        }
    }

    else
    {
        printf("No filter specified. Receiving ALL messages.\n");
    }

    /*
     * Select vcan0
     */
    strcpy(interface_request.ifr_name, "vcan0");

    if (ioctl(socket_fd,
              SIOCGIFINDEX,
              &interface_request) < 0)
    {
        perror("Could not find vcan0");
        close(socket_fd);
        return 1;
    }

    /*
     * Bind socket to vcan0
     */
    address.can_family = AF_CAN;
    address.can_ifindex = interface_request.ifr_ifindex;

    if (bind(socket_fd,
             (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("CAN socket bind failed");
        close(socket_fd);
        return 1;
    }

    printf("Dashboard ECU started...\n\n");

    /*
     * -----------------------------
     * MAIN RECEIVE LOOP
     * -----------------------------
     */

    while (1)
    {
        fd_set read_fds;
        struct timeval timeout;

        FD_ZERO(&read_fds);
        FD_SET(socket_fd, &read_fds);

        /*
         * Wait for CAN message for 1 second
         */
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int result = select(socket_fd + 1,
                            &read_fds,
                            NULL,
                            NULL,
                            &timeout);

        if (result < 0)
        {
            perror("select failed");
            continue;
        }

        /*
         * CAN message received
         */
        if (result > 0 && FD_ISSET(socket_fd, &read_fds))
        {
            if (read(socket_fd, &frame, sizeof(frame)) < 0)
            {
                perror("CAN frame reception failed");
                continue;
            }

            /*
             * -----------------------------
             * SPEED MESSAGE
             * CAN ID = 0x100
             * -----------------------------
             */

            if (frame.can_id == 0x100 &&
                frame.can_dlc >= 2)
            {
                speed = (frame.data[0] << 8) |
                        frame.data[1];

                /*
                 * Update last Speed reception time
                 */
                last_speed_time = time(NULL);

                /*
                 * Vehicle is communicating again
                 */
                if (vehicle_offline)
                {
                    printf("\n");
                    printf("Vehicle ECU communication restored.\n");
                    printf("\n");

                    vehicle_offline = 0;
                }

                printf("Speed received: %d km/h\n",
                       speed);
            }

            /*
             * -----------------------------
             * RPM MESSAGE
             * CAN ID = 0x101
             * -----------------------------
             */

            else if (frame.can_id == 0x101 &&
                     frame.can_dlc >= 2)
            {
                rpm = (frame.data[0] << 8) |
                      frame.data[1];

                printf("RPM received: %d rpm\n",
                       rpm);
            }

            /*
             * -----------------------------
             * TEMPERATURE MESSAGE
             * CAN ID = 0x102
             * -----------------------------
             */

            else if (frame.can_id == 0x102 &&
                     frame.can_dlc >= 1)
            {
                temperature = frame.data[0];

                printf("Temperature received: %d C\n",
                       temperature);
            }

            /*
             * -----------------------------
             * UNKNOWN CAN MESSAGE
             * -----------------------------
             */

            else
            {
                printf("Unknown CAN ID: 0x%03X\n",
                       frame.can_id);
            }
        }

        /*
         * -----------------------------
         * VEHICLE ECU OFFLINE CHECK
         * -----------------------------
         *
         * If no Speed message is received
         * for 2 seconds, declare ECU offline.
         */

        if (difftime(time(NULL),
                     last_speed_time) >= 2)
        {
            if (!vehicle_offline)
            {
                printf("\n");
                printf("====================================\n");
                printf("WARNING: Vehicle ECU Offline\n");
                printf("No Speed message received for 2 seconds.\n");
                printf("====================================\n");
                printf("\n");

                vehicle_offline = 1;
            }
        }
    }

    close(socket_fd);

    return 0;
}
