#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>

int main(void)
{
    int socket_fd;
    struct sockaddr_can address;
    struct ifreq interface_request;
    struct canfd_frame frame;

    /*
     * Create CAN RAW socket
     */
    socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    if (socket_fd < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    /*
     * Enable CAN FD frames
     */
    int enable_canfd = 1;

    if (setsockopt(socket_fd,
                   SOL_CAN_RAW,
                   CAN_RAW_FD_FRAMES,
                   &enable_canfd,
                   sizeof(enable_canfd)) < 0)
    {
        perror("CAN FD enable failed");
        close(socket_fd);
        return 1;
    }

    /*
     * Select vcanfd0
     */
    strcpy(interface_request.ifr_name, "vcanfd0");

    if (ioctl(socket_fd,
              SIOCGIFINDEX,
              &interface_request) < 0)
    {
        perror("Could not find vcanfd0");
        close(socket_fd);
        return 1;
    }

    /*
     * Bind socket
     */
    address.can_family = AF_CAN;
    address.can_ifindex = interface_request.ifr_ifindex;

    if (bind(socket_fd,
             (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("CAN FD socket bind failed");
        close(socket_fd);
        return 1;
    }

    printf("CAN FD Receiver started on vcanfd0...\n");
    printf("Waiting for CAN FD frames...\n\n");

    while (1)
    {
        int bytes_received;

        bytes_received = read(socket_fd,
                              &frame,
                              sizeof(frame));

        if (bytes_received < 0)
        {
            perror("CAN FD reception failed");
            continue;
        }

        printf("CAN FD Frame Received\n");
        printf("CAN ID : 0x%03X\n",
               frame.can_id);

        printf("DLC    : %d\n",
               frame.len);

        printf("Data   : ");

        for (int i = 0; i < frame.len; i++)
        {
            printf("%02X ", frame.data[i]);
        }

        printf("\n");
        printf("--------------------------------\n");
    }

    close(socket_fd);

    return 0;
}
