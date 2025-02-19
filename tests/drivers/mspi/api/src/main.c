/*
 * Copyright (c) 2024 Ambiq Micro Inc. <www.ambiq.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/mspi_emul.h>
#include <zephyr/ztest.h>

#define TEST_MSPI_REINIT    1

#define MSPI_BUS_NODE       DT_ALIAS(mspi0)

#define NRF_GPIOHSPADCTRL_S_BASE 0x50050400UL
#define NRF_GPIOHSPADCTRL ((NRF_GPIOHSPADCTRL_Type *)NRF_GPIOHSPADCTRL_S_BASE)

#define BUF_SIZE 256
uint8_t memc_write_buffer[BUF_SIZE];

static const struct device *mspi_devices[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(MSPI_BUS_NODE, DEVICE_DT_GET, (,))
};

static struct gpio_dt_spec ce_gpios[] = MSPI_CE_GPIOS_DT_SPEC_GET(MSPI_BUS_NODE);

#if TEST_MSPI_REINIT
struct mspi_cfg hardware_cfg = {
	.channel_num              = 0,
	.op_mode                  = DT_PROP_OR(MSPI_BUS_NODE, op_mode, MSPI_OP_MODE_CONTROLLER),
	.duplex                   = DT_PROP_OR(MSPI_BUS_NODE, duplex, MSPI_HALF_DUPLEX),
	.dqs_support              = DT_PROP_OR(MSPI_BUS_NODE, dqs_support, false),
	.ce_group                 = ce_gpios,
	.num_ce_gpios             = ARRAY_SIZE(ce_gpios),
	.num_periph               = DT_CHILD_NUM(MSPI_BUS_NODE),
	.max_freq                 = DT_PROP(MSPI_BUS_NODE, clock_frequency),
	.re_init                  = true,
};
#endif


static struct mspi_dev_id dev_id[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(MSPI_BUS_NODE, MSPI_DEVICE_ID_DT, (,))
};

static struct mspi_dev_cfg device_cfg[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(MSPI_BUS_NODE, MSPI_DEVICE_CONFIG_DT, (,))
};


struct mspi_xfer_packet packet1[] = {
{
	.dir                = MSPI_TX, // write enable
	.cmd                = 0x06,
	.address            = 0x00,
	.num_bytes          = 0,
	.data_buf           = memc_write_buffer,
	.cb_mask            = MSPI_BUS_NO_CB,
},
{
	.dir                = MSPI_TX, //quad enable
	.cmd                = 0x01,
	.address            = 0x00,
	.num_bytes          = 1,
	.data_buf           = memc_write_buffer,
	.cb_mask            = MSPI_BUS_NO_CB,
},
// {
// 	.dir                = MSPI_TX,
// 	.cmd                = 0x01,
// 	.address            = 0,
// 	.num_bytes          = 3,
// 	.data_buf           = memc_write_buffer,
// 	.cb_mask            = MSPI_BUS_NO_CB,
// },
// {
// 	.dir                = MSPI_RX,
// 	.cmd                = 0x15,
// 	.address            = 0,
// 	.num_bytes          = 2,
// 	.data_buf           = memc_write_buffer,
// 	.cb_mask            = MSPI_BUS_NO_CB,
// },
// {
// 	.dir                = MSPI_RX,
// 	.cmd                = 0x01,
// 	.address            = 0,
// 	.num_bytes          = 3,
// 	.data_buf           = memc_write_buffer,
// 	.cb_mask            = MSPI_BUS_NO_CB,
// },
};

struct mspi_xfer_packet packet2a[] = {
{
	.dir                = MSPI_RX,
	.cmd                = 0x05, // status read
	.address            = 0,
	.num_bytes          = 1,
	.data_buf           = memc_write_buffer + 10,
	.cb_mask            = MSPI_BUS_NO_CB,
},
};

struct mspi_xfer_packet packet2b[] = {
	{
		.dir                = MSPI_TX, // write enable, again
		.cmd                = 0x06,
		.address            = 0x00,
		.num_bytes          = 0,
		.data_buf           = memc_write_buffer,
		.cb_mask            = MSPI_BUS_NO_CB,
	},
};

struct mspi_xfer_packet packet2[] = {
{
	.dir                = MSPI_TX,
	.cmd                = 0x02, // page program
	.address            = 0,
	.num_bytes          = 3,
	.data_buf           = memc_write_buffer + 2,
	.cb_mask            = MSPI_BUS_NO_CB,
},
// {
// 	.dir                = MSPI_RX,
// 	.cmd                = 0x6B,
// 	.address            = 0,
// 	.num_bytes          = 3,
// 	.data_buf           = memc_write_buffer + 4,
// 	.cb_mask            = MSPI_BUS_NO_CB,
// },
};

struct mspi_xfer_packet packet3[] = {
// {
// 	.dir                = MSPI_TX,
// 	.cmd                = 0x02,
// 	.address            = 0,
// 	.num_bytes          = 3,
// 	.data_buf           = memc_write_buffer + 1,
// 	.cb_mask            = MSPI_BUS_NO_CB,
// },
{
	.dir                = MSPI_RX,
	.cmd                = 0x6B,
	.address            = 0,
	.num_bytes          = 6,
	.data_buf           = memc_write_buffer + 5,
	.cb_mask            = MSPI_BUS_NO_CB,
},
};

struct mspi_xfer xfer1 = {
	.async                      = false,
	.xfer_mode                  = MSPI_PIO,
	.tx_dummy                   = 0,
	.cmd_length                 = 1,
	.addr_length                = 0,
	.priority                   = 0,
	.packets                    = (struct mspi_xfer_packet *)&packet1,
	.num_packet                 = 2, //sizeof(packet1) / sizeof(struct mspi_xfer_packet),
	.timeout                    = 100,
};

struct mspi_xfer xfer2a = {
	.async                      = false,
	.xfer_mode                  = MSPI_PIO,
	.tx_dummy                   = 0,
	.cmd_length                 = 1,
	.addr_length                = 0,
	.priority                   = 0,
	.packets                    = (struct mspi_xfer_packet *)&packet2a,
	.num_packet                 = 1, //sizeof(packet1) / sizeof(struct mspi_xfer_packet),
	.timeout                    = 100,
};

struct mspi_xfer xfer2b = {
	.async                      = false,
	.xfer_mode                  = MSPI_PIO,
	.tx_dummy                   = 0,
	.cmd_length                 = 1,
	.addr_length                = 0,
	.priority                   = 0,
	.packets                    = (struct mspi_xfer_packet *)&packet2b,
	.num_packet                 = 1, //sizeof(packet1) / sizeof(struct mspi_xfer_packet),
	.timeout                    = 100,
};

struct mspi_xfer xfer2 = {
	.async                      = false,
	.xfer_mode                  = MSPI_PIO,
	.tx_dummy                   = 0,
	.cmd_length                 = 1,
	.addr_length                = 3,
	.priority                   = 0,
	.packets                    = (struct mspi_xfer_packet *)&packet2,
	.num_packet                 = 1, //sizeof(packet1) / sizeof(struct mspi_xfer_packet),
	.timeout                    = 100,
};

struct mspi_xfer xfer3 = {
	.async                      = false,
	.xfer_mode                  = MSPI_PIO,
	.tx_dummy                   = 0,
	.cmd_length                 = 1,
	.addr_length                = 4,
	.priority                   = 0,
	.packets                    = (struct mspi_xfer_packet *)&packet3,
	.num_packet                 = 1, //sizeof(packet1) / sizeof(struct mspi_xfer_packet),
	.timeout                    = 100,
};

ZTEST(mspi_api, test_mspi_api)
{
	int ret = 0;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	zassert_true(device_is_ready(mspi_bus), "mspi_bus is not ready");

	/* Initialize write buffer */
	for (uint32_t i = 0; i < BUF_SIZE; i++) {
		memc_write_buffer[i] = (uint8_t)i+1;
	}

#if TEST_MSPI_REINIT
	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = hardware_cfg,
	};

	ret = mspi_config(&spec);
	zassert_equal(ret, 0, "mspi_config failed.");
#endif


	NRF_GPIOHSPADCTRL_Type * hsgpio = NRF_GPIOHSPADCTRL;

	printf("hsgpio value: %d\n", hsgpio->BIAS);

	for (int dev_idx = 0; dev_idx < ARRAY_SIZE(mspi_devices); ++dev_idx) {

		zassert_true(device_is_ready(mspi_devices[dev_idx]), "mspi_device is not ready");

		ret = mspi_dev_config(mspi_bus, &dev_id[dev_idx],
						MSPI_DEVICE_CONFIG_ALL, &device_cfg[dev_idx]);
		zassert_equal(ret, 0, "mspi_dev_config failed.");

		memc_write_buffer[0] = 0x42;

		/* Enable write, enable quad. */

		ret = mspi_transceive(mspi_bus, &dev_id[dev_idx], &xfer1);
		printf("ret = %d\n", ret);

		k_sleep(K_MSEC(20));

		/* Read Status Register. */

		ret = mspi_transceive(mspi_bus, &dev_id[dev_idx], &xfer2a);
		printf("ret = %d\n", ret);

		printf("status:%x\n", memc_write_buffer[10]);

		/* Write on QUAD. */
		memc_write_buffer[2] = 0xc2;
		memc_write_buffer[3] = 0x28;
		memc_write_buffer[4] = 0x17;

		device_cfg[dev_idx].io_mode = MSPI_IO_MODE_QUAD_1_1_4;

		ret = mspi_dev_config(mspi_bus, &dev_id[dev_idx],
						MSPI_DEVICE_CONFIG_IO_MODE , &device_cfg[dev_idx]);
		zassert_equal(ret, 0, "mspi_dev_config2 failed.");

		ret = mspi_transceive(mspi_bus, &dev_id[dev_idx], &xfer2);
		printf("ret = %d\n", ret);


		/* Enable write again, so that page program would work. */

		// ret = mspi_transceive(mspi_bus, &dev_id[dev_idx], &xfer2b);
		// printf("ret = %d\n", ret);

		// k_sleep(K_MSEC(5));

		/* Page program. */

		// memc_write_buffer[2] = 0xf0;
		// memc_write_buffer[3] = 0xff;
		// memc_write_buffer[4] = 0x0f;

		// memc_write_buffer[2] = 0xc2;
		// memc_write_buffer[3] = 0x28;
		// memc_write_buffer[4] = 0x17;

		// ret = mspi_transceive(mspi_bus, &dev_id[dev_idx], &xfer2);
		// printf("ret = %d\n", ret);

		// k_sleep(K_MSEC(10));

		/* Quad read. */

		// device_cfg[dev_idx].io_mode = MSPI_IO_MODE_QUAD_1_1_4;

		// ret = mspi_dev_config(mspi_bus, &dev_id[dev_idx],
		// 				MSPI_DEVICE_CONFIG_IO_MODE , &device_cfg[dev_idx]);
		// zassert_equal(ret, 0, "mspi_dev_config2 failed.");

		// ret = mspi_transceive(mspi_bus, &dev_id[dev_idx], &xfer3);
		// printf("ret = %d\n", ret);

		// printf("D:%x %x, %x, %x\n", memc_write_buffer[5], memc_write_buffer[6], memc_write_buffer[7], memc_write_buffer[8]);
		// printf("D2:%x %x, %x, %x\n", memc_write_buffer[8], memc_write_buffer[9], memc_write_buffer[10], memc_write_buffer[11]);

		/* Read Status Register. */

		// device_cfg[dev_idx].io_mode = MSPI_IO_MODE_SINGLE;

		// ret = mspi_dev_config(mspi_bus, &dev_id[dev_idx],
		// 				MSPI_DEVICE_CONFIG_IO_MODE , &device_cfg[dev_idx]);
		// zassert_equal(ret, 0, "mspi_dev_config3 failed.");

		// ret = mspi_transceive(mspi_bus, &dev_id[dev_idx], &xfer2a);
		// printf("ret = %d\n", ret);

		// printf("status:%x\n", memc_write_buffer[10]);
	}

	k_sleep(K_MSEC(1000));
}

ZTEST_SUITE(mspi_api, NULL, NULL, NULL, NULL, NULL);
