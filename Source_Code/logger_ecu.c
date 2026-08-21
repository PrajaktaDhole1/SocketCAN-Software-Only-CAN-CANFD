#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>

int main()
{
    int socket_fd;
    struct sockaddr_can address;
    struct ifreq interface_request;
    struct can_frame frame;

    /* Create CAN socket */
    socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    if (socket_fd < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    /* Select vcan0 */
    strcpy(interface_request.ifr_name, "vcan0");

    if (ioctl(socket_fd, SIOCGIFINDEX, &interface_request) < 0)
    {
        perror("Could not find vcan0");
        close(socket_fd);
        return 1;
    }

    /* Bind socket to vcan0 */
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

    /* Open CSV log file */
    FILE *log_file = fopen("can_log.csv", "a");

    if (log_file == NULL)
    {
        perror("Could not open log file");
        close(socket_fd);
        return 1;
    }

    /* Write CSV header if file is empty */
    fseek(log_file, 0, SEEK_END);

    if (ftell(log_file) == 0)
    {
        fprintf(log_file, "Timestamp,CAN_ID,DLC,Data\n");
        fflush(log_file);
    }

    printf("Logger ECU started...\n");
    printf("Logging CAN traffic to can_log.csv\n\n");

    while (1)
    {
        /* Receive CAN frame */
        if (read(socket_fd, &frame, sizeof(frame)) < 0)
        {
            perror("CAN frame reception failed");
            continue;
        }

        /* Get current time */
        time_t current_time = time(NULL);
        struct tm *time_info = localtime(&current_time);

        char timestamp[30];

        strftime(timestamp,
                 sizeof(timestamp),
                 "%Y-%m-%d %H:%M:%S",
                 time_info);

        /* Print received message */
        printf("Logged -> %s | ID: 0x%03X | DLC: %d | Data:",
               timestamp,
               frame.can_id,
               frame.can_dlc);

        /* Write timestamp, ID and DLC */
        fprintf(log_file,
                "%s,0x%03X,%d,",
                timestamp,
                frame.can_id,
                frame.can_dlc);

        /* Write CAN data */
        for (int i = 0; i < frame.can_dlc; i++)
        {
            printf(" %02X", frame.data[i]);

            fprintf(log_file,
                    "%02X",
                    frame.data[i]);

            if (i < frame.can_dlc - 1)
                fprintf(log_file, " ");
        }

        fprintf(log_file, "\n");

        /* Make sure data is immediately written */
        fflush(log_file);

        printf("\n");
    }

    fclose(log_file);
    close(socket_fd);

    return 0;
}
