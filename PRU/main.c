#include <stdint.h>
#include <pru_intc.h>
#include <pru_rpmsg.h>
#include "resource_table_0.h"

extern void asm_count(uint32_t *data_ptr);

volatile register uint32_t __R31;

// Host-0 Interrupt sets bit 30 in register R31
#define HOST_INT            ((uint32_t) 1 << 30)

/* The PRU-ICSS system events used for RPMsg are defined in the Linux device tree
 * PRU0 uses system event 16 (To ARM) and 17 (From ARM)
 * PRU1 uses system event 18 (To ARM) and 19 (From ARM)
 */
#define TO_ARM_HOST         16
#define FROM_ARM_HOST       17

/*
 * Using the name 'rpmsg-pru' will probe the rpmsg_pru driver found
 * at linux-x.y.z/drivers/rpmsg/rpmsg_pru.c
 */
#define CHAN_NAME           "rpmsg-pru"
#define CHAN_DESC           "Channel 30"
#define CHAN_PORT           30

/*
 * Used to make sure the Linux drivers are ready for RPMsg communication
 * Found at linux-x.y.z/include/uapi/linux/virtio_config.h
 */
#define VIRTIO_CONFIG_S_DRIVER_OK   4

uint8_t payload[RPMSG_BUF_SIZE];

void main(void)
{
    struct pru_rpmsg_transport transport;
    uint16_t src, dst, len;
    volatile uint8_t *status;
    uint32_t data[4];

    // Clear the status of the PRU-ICSS system event that the ARM will use to 'kick' us
    CT_INTC.SICR_bit.STS_CLR_IDX = FROM_ARM_HOST;

    // Make sure the Linux drivers are ready for RPMsg communication
    status = &resourceTable.rpmsg_vdev.status;
    while (!(*status & VIRTIO_CONFIG_S_DRIVER_OK));

    // Initialize the RPMsg transport structure */
    pru_rpmsg_init(&transport, &resourceTable.rpmsg_vring0, &resourceTable.rpmsg_vring1, TO_ARM_HOST, FROM_ARM_HOST);

    // Create the RPMsg channel between the PRU and ARM user space using the transport structure.
    while (pru_rpmsg_channel(RPMSG_NS_CREATE, &transport, CHAN_NAME, CHAN_DESC, CHAN_PORT) != PRU_RPMSG_SUCCESS);
    while (1) {
        if (__R31 & HOST_INT) {
            CT_INTC.SICR_bit.STS_CLR_IDX = FROM_ARM_HOST;
            while (pru_rpmsg_receive(&transport, &src, &dst, payload, &len) == PRU_RPMSG_SUCCESS) {
                asm_count(&data);
                pru_rpmsg_send(&transport, dst, src, data, 4);
            }
        }
    }
}
