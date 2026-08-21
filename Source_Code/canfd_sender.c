#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
     * Bind socket to vcanfd0
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

    printf("====================================\n");
    printf("       CAN FD Sender Started\n");
    printf("====================================\n");
    printf("Interface : vcanfd0\n");
    printf("CAN ID    : 0x123\n");
    printf("Payload   : 20 bytes\n");
    printf("====================================\n\n");

    /*
     * Prepare CAN FD frame
     */
    memset(&frame, 0, sizeof(frame));

    frame.can_id = 0x123;

    frame.len = 20;

    /*
     * 20-byte payload
     */
    unsigned char data[20] =
    {
        0x11, 0x22, 0x33, 0x44,
        0x55, 0x66, 0x77, 0x88,
        0x99, 0xAA, 0xBB, 0xCC,
        0xDD, 0xEE, 0xFF, 0x00,
        0x12, 0x34, 0x56, 0x78
    };

    memcpy(frame.data, data, 20);

    /*
     * Send CAN FD frame
     */
    int bytes_sent;

    bytes_sent = write(socket_fd,
                       &frame,
                       CANFD_MTU);

    if (bytes_sent != CANFD_MTU)
    {
        perror("CAN FD frame transmission failed");
        close(socket_fd);
        return 1;
    }

    printf("CAN FD frame transmitted successfully!\n");

    printf("CAN ID : 0x%03X\n",
           frame.can_id);

    printf("Length : %d bytes\n",
           frame.len);

    printf("Data   : ");

    for (int i = 0; i < frame.len; i++)
    {
        printf("%02X ", frame.data[i]);
    }

    printf("\n");

    close(socket_fd);

    return 0;
}
