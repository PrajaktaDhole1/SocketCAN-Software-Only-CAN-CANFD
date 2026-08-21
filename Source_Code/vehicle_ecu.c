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

    // Create CAN socket
    socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    if (socket_fd < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    // Specify CAN interface
    strcpy(interface_request.ifr_name, "vcan0");

    if (ioctl(socket_fd, SIOCGIFINDEX, &interface_request) < 0)
    {
        perror("Could not find vcan0");
        close(socket_fd);
        return 1;
    }

    // Bind socket to vcan0
    address.can_family = AF_CAN;
    address.can_ifindex = interface_request.ifr_ifindex;

    if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("CAN socket bind failed");
        close(socket_fd);
        return 1;
    }

    printf("Vehicle ECU started...\n");
    printf("Sending CAN messages on vcan0\n\n");

    int speed = 40;
    int rpm = 1500;
    int temperature = 70;

    while (1)
    {
        /* -----------------------------
           SPEED MESSAGE - CAN ID 0x100
           ----------------------------- */

        frame.can_id = 0x100;
        frame.can_dlc = 2;

        frame.data[0] = (speed >> 8) & 0xFF;
        frame.data[1] = speed & 0xFF;

        if (write(socket_fd, &frame, sizeof(frame)) != sizeof(frame))
        {
            perror("Speed message transmission failed");
        }

        /* -----------------------------
           RPM MESSAGE - CAN ID 0x101
           ----------------------------- */

        frame.can_id = 0x101;
        frame.can_dlc = 2;

        frame.data[0] = (rpm >> 8) & 0xFF;
        frame.data[1] = rpm & 0xFF;

        if (write(socket_fd, &frame, sizeof(frame)) != sizeof(frame))
        {
            perror("RPM message transmission failed");
        }

        /* -----------------------------
           TEMPERATURE MESSAGE - CAN ID 0x102
           ----------------------------- */

        frame.can_id = 0x102;
        frame.can_dlc = 1;

        frame.data[0] = temperature;

        if (write(socket_fd, &frame, sizeof(frame)) != sizeof(frame))
        {
            perror("Temperature message transmission failed");
        }

        printf("Sent -> Speed: %d km/h | RPM: %d | Temperature: %d C\n",
               speed, rpm, temperature);

        /* Make values change realistically */

        speed += 5;
        rpm += 200;
        temperature += 1;

        if (speed > 120)
            speed = 40;

        if (rpm > 5000)
            rpm = 1500;

        if (temperature > 120)
            temperature = 70;

        /* Wait 1 second */

        sleep(1);
    }

    close(socket_fd);

    return 0;
}
