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


const struct mspi_xfer_packet packet1[] = {
{
	.dir                = MSPI_RX,
	.cmd                = 0x9F,
	.address            = 0x00,
	.num_bytes          = 3,
	.data_buf           = memc_write_buffer,
	.cb_mask            = MSPI_BUS_NO_CB,
},
{
	.dir                = MSPI_TX,
	.cmd                = 0x55,
	.address            = 64,
	.num_bytes          = 64,
	.data_buf           = memc_write_buffer + 64,
	.cb_mask            = MSPI_BUS_NO_CB,
},
{
	.dir                = MSPI_TX,
	.cmd                = 0xAA,
	.address            = 128,
	.num_bytes          = 64,
	.data_buf           = memc_write_buffer + 128,
	.cb_mask            = MSPI_BUS_NO_CB,
},
{
	.dir                = MSPI_RX,
	.cmd                = 0x55,
	.address            = 128 + 64,
	.num_bytes          = 64,
	.data_buf           = memc_write_buffer + 128 + 64,
	.cb_mask            = MSPI_BUS_NO_CB,
},
};

struct mspi_xfer xfer = {
	.async                      = false,
	.xfer_mode                  = MSPI_PIO,
	.tx_dummy                   = 0,
	.cmd_length                 = 1,
	.addr_length                = 0,
	.priority                   = 0,
	.packets                    = (struct mspi_xfer_packet *)&packet1,
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

		ret = mspi_transceive(mspi_bus, &dev_id[dev_idx], &xfer);
		printf("ret = %d\n", ret);

		printf("len:%d D: %x, %x, %x\n", memc_write_buffer[0], memc_write_buffer[1], memc_write_buffer[2], memc_write_buffer[3]);

	}
}

ZTEST_SUITE(mspi_api, NULL, NULL, NULL, NULL, NULL);
