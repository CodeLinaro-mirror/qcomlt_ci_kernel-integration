// SPDX-License-Identifier: GPL-2.0

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <dt-bindings/clock/qcom,audiocc-sm8250.h>

struct lpasscc_sm8250 {
	struct device *dev;
	void __iomem *audiocc;
	void __iomem *aocc;
};

struct clk_gfm {
	int id;
	unsigned int mux_reg;
	unsigned int mux_mask;
	struct clk_hw	hw;
	struct lpasscc_sm8250 *priv;
	void __iomem *gfm_mux;
};

#define GFM_MASK	BIT(1)
#define to_clk_gfm(_hw) container_of(_hw, struct clk_gfm, hw)

static u8 clk_gfm_get_parent(struct clk_hw *hw)
{
	struct clk_gfm *clk = to_clk_gfm(hw);

	return readl(clk->gfm_mux) & GFM_MASK;
}

static int clk_gfm_set_parent(struct clk_hw *hw, u8 index)
{
	struct clk_gfm *clk = to_clk_gfm(hw);
	unsigned int val;

	val = readl(clk->gfm_mux);

	if (index)
		val |= GFM_MASK;
	else
		val &= ~GFM_MASK;

	writel(val, clk->gfm_mux);

	return 0;
}

static const struct clk_ops clk_gfm_ops = {
	.get_parent = clk_gfm_get_parent,
	.set_parent = clk_gfm_set_parent,
	.determine_rate = __clk_mux_determine_rate,
};

static struct clk_gfm lpass_gfm_va_mclk = {
	.mux_reg = 0x20000,
	.mux_mask = BIT(0),
	.hw.init = &(struct clk_init_data) {
		.name = "VA_MCLK",
		.ops = &clk_gfm_ops,
		.flags = CLK_SET_RATE_PARENT,
		.parent_names = (const char *[]) {
			"LPASS_CLK_ID_TX_CORE_MCLK",
			"LPASS_CLK_ID_VA_CORE_MCLK",
		},
		.num_parents = 2,
		.parent_data = (const struct clk_parent_data[]){
				{ .index = 0 },
				{ .index = 1 },
		},
	},
};

static struct clk_gfm lpass_gfm_tx_npl = {
	.mux_reg = 0x20000,
	.mux_mask = BIT(0),
	.hw.init = &(struct clk_init_data) {
		.name = "TX_NPL",
		.ops = &clk_gfm_ops,
		.flags = CLK_SET_RATE_PARENT,
		.parent_names = (const char *[]){
			"LPASS_CLK_ID_TX_CORE_NPL_MCLK",
			"LPASS_CLK_ID_VA_CORE_2X_MCLK",
		},
		.parent_data = (const struct clk_parent_data[]){
				{ .index = 0 },
				{ .index = 1 },
		},
		.num_parents = 2,
	},
};

static struct clk_gfm lpass_gfm_wsa_mclk = {
	.mux_reg = 0x220d8,
	.mux_mask = BIT(0),
	.hw.init = &(struct clk_init_data) {
		.name = "WSA_MCLK",
		.ops = &clk_gfm_ops,
		.flags = CLK_SET_RATE_PARENT,
		.parent_names = (const char *[]){
			"LPASS_CLK_ID_TX_CORE_MCLK",
			"LPASS_CLK_ID_WSA_CORE_MCLK",
		},
		.parent_data = (const struct clk_parent_data[]){
				{ .index = 0 },
				{ .index = 1 },
		},
		.num_parents = 2,
	},
};

static struct clk_gfm lpass_gfm_wsa_npl = {
	.mux_reg = 0x220d8,
	.mux_mask = BIT(0),
	.hw.init = &(struct clk_init_data) {
		.name = "WSA_NPL",
		.ops = &clk_gfm_ops,
		.flags = CLK_SET_RATE_PARENT,
		.parent_names = (const char *[]){
			"LPASS_CLK_ID_TX_CORE_NPL_MCLK",
			"LPASS_CLK_ID_WSA_CORE_NPL_MCLK",
		},
		.parent_data = (const struct clk_parent_data[]){
				{ .index = 0 },
				{ .index = 1 },
		},
		.num_parents = 2,
	},
};

static struct clk_gfm lpass_gfm_rx_mclk_mclk2 = {
	.mux_reg = 0x240d8,
	.mux_mask = BIT(0),
	.hw.init = &(struct clk_init_data) {
		.name = "RX_MCLK_MCLK2",
		.ops = &clk_gfm_ops,
		.flags = CLK_SET_RATE_PARENT,
		.parent_names = (const char *[]){
			"LPASS_CLK_ID_TX_CORE_MCLK",
			"LPASS_CLK_ID_RX_CORE_MCLK",
		},
		.parent_data = (const struct clk_parent_data[]){
				{ .index = 0 },
				{ .index = 1 },
		},
		.num_parents = 2,
	},
};

static struct clk_gfm lpass_gfm_rx_npl = {
	.mux_reg = 0x240d8,
	.mux_mask = BIT(0),
	.hw.init = &(struct clk_init_data) {
		.name = "RX_NPL",
		.ops = &clk_gfm_ops,
		.flags = CLK_SET_RATE_PARENT,
		.parent_names = (const char *[]){
			"LPASS_CLK_ID_TX_CORE_NPL_MCLK",
			"LPASS_CLK_ID_RX_CORE_NPL_MCLK",
		},
		.parent_data = (const struct clk_parent_data[]){
				{ .index = 0 },
				{ .index = 1 },
		},
		.num_parents = 2,
	},
};

static struct clk_gfm *lpass_gfm_clks[] = {
	[LPASS_CDC_VA_MCLK]		= &lpass_gfm_va_mclk,
	[LPASS_CDC_TX_NPL]		= &lpass_gfm_tx_npl,
//	[LPASS_CDC_TX_MCLK]		= &lpass_gfm_tx_mclk
	[LPASS_CDC_WSA_NPL]		= &lpass_gfm_wsa_npl,
	[LPASS_CDC_WSA_MCLK]		= &lpass_gfm_wsa_mclk,
//	[LPASS_CDC_RX_MCLK]		= &lpass_gfm_rx_mclk_mclk2,
	[LPASS_CDC_RX_NPL]		= &lpass_gfm_rx_npl,
	[LPASS_CDC_RX_MCLK_MCLK2]	= &lpass_gfm_rx_mclk_mclk2,
};

static struct clk_hw_onecell_data lpasscc_hw_onecell_data = {
        .hws = {
		[LPASS_CDC_VA_MCLK]	= &lpass_gfm_va_mclk.hw,
		[LPASS_CDC_TX_NPL]	= &lpass_gfm_tx_npl.hw,
//		[LPASS_CDC_TX_MCLK]	= &lpass_gfm_tx_mclk.hw,
		[LPASS_CDC_WSA_NPL]	= &lpass_gfm_wsa_npl.hw,
		[LPASS_CDC_WSA_MCLK]	= &lpass_gfm_wsa_mclk.hw,
//		[LPASS_CDC_RX_MCLK]	= &lpass_gfm_rx_mclk_mclk2.hw
		[LPASS_CDC_RX_NPL]	= &lpass_gfm_rx_npl.hw,
		[LPASS_CDC_RX_MCLK_MCLK2] = &lpass_gfm_rx_mclk_mclk2.hw,
	},
        .num = ARRAY_SIZE(lpass_gfm_clks),
};

extern bool q6core_is_adsp_ready(void);

static int lpasscc_sm8250_clk_driver_probe(struct platform_device *pdev)
{
	struct lpasscc_sm8250 *lpasscc;
	struct device *dev = &pdev->dev;
	struct clk_gfm *gfm;
	struct resource *res;
	struct clk *lpass_core_hw_vote = NULL;
	int err, i;

	if (!q6core_is_adsp_ready())
		return -EPROBE_DEFER;

	lpass_core_hw_vote = devm_clk_get(&pdev->dev, "core");
	if (IS_ERR(lpass_core_hw_vote)) {
		dev_dbg(&pdev->dev, "%s: clk get %s failed\n",
			__func__, "lpass_core_hw_vote");
		return PTR_ERR(lpass_core_hw_vote);
	}

	clk_prepare_enable(lpass_core_hw_vote);

	lpasscc = devm_kzalloc(dev, sizeof(*lpasscc), GFP_KERNEL);
	if (!lpasscc)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	lpasscc->audiocc = devm_ioremap_resource(dev, res);
	if (IS_ERR(lpasscc->audiocc))
		return PTR_ERR(lpasscc->audiocc);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	lpasscc->aocc = devm_ioremap_resource(dev, res);
	if (IS_ERR(lpasscc->aocc))
		return PTR_ERR(lpasscc->aocc);

	for (i = 0; i < ARRAY_SIZE(lpass_gfm_clks); i++) {
		if (!lpass_gfm_clks[i])
			continue;

		gfm = lpass_gfm_clks[i];
		gfm->priv = lpasscc;

		if (i >= LPASS_CDC_WSA_NPL)
			gfm->gfm_mux = lpasscc->audiocc;
		else
			gfm->gfm_mux = lpasscc->aocc;

		gfm->gfm_mux = gfm->gfm_mux + lpass_gfm_clks[i]->mux_reg;

		gfm->id = i;
		err = devm_clk_hw_register(dev, &lpass_gfm_clks[i]->hw);
		if (err)
			return err;

	}

	return devm_of_clk_add_hw_provider(dev, of_clk_hw_onecell_get,
					   &lpasscc_hw_onecell_data);
}

static const struct of_device_id lpasscc_sm8250_clk_match_table[] = {
	{ .compatible = "qcom,sm8250-audiocc" },
	{ }
};
MODULE_DEVICE_TABLE(of, lpasscc_sm8250_clk_match_table);

static struct platform_driver lpasscc_sm8250_clk_driver = {
	.probe		= lpasscc_sm8250_clk_driver_probe,
	.driver		= {
		.name	= "lpass-gfm-clk",
		.of_match_table = lpasscc_sm8250_clk_match_table,
	},
};
module_platform_driver(lpasscc_sm8250_clk_driver);
MODULE_LICENSE("GPL v2");
