/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <linux/delay.h>
#include <linux/pwm.h>
#include <mach/gpio.h>
#include "msm_fb.h"

#define DEFAULT_BRIGHTNESS	158

struct lcdc_auo_data {
	struct msm_panel_common_pdata *pdata;
	struct platform_device *fbpdev;
};

static struct lcdc_auo_data *dd;

static void lcdc_auo_panel_set_backlight(struct msm_fb_data_type *mfd);

static int bl_level_prevset = 1;

static void lcdc_auo_panel_bkl_switch(struct msm_fb_data_type *mfd, bool on)
{
	unsigned int val = 0;

	pr_info("lcdc_auo_panel_bkl_switch, on=%d\n", on);
	if(on) {
		val = mfd->bl_level;
		if(val == 0) {
			if(bl_level_prevset != 1) {
				val = bl_level_prevset;
				mfd->bl_level = val;
			} else {
				val = DEFAULT_BRIGHTNESS;
				mfd->bl_level = val;
			}
		}
		lcdc_auo_panel_set_backlight(mfd);
	} else {
		mfd->bl_level = 0;
		lcdc_auo_panel_set_backlight(mfd);
	}

	return;
}

static void lcdc_auo_panel_bkl_ctrl(bool on)
{
	pr_info("lcdc_auo_panel_bkl_ctrl, on=%d\n", on);

	return;
}

static void lcdc_auo_panel_set_backlight(struct msm_fb_data_type *mfd)
{
	int bl_level = mfd->bl_level;

	if ((bl_level & 0xf)==0)
		pr_info("wxga %s: %d\n", __func__, bl_level);

	if (dd->pdata->pmic_backlight && dd->pdata->shrink_pwm)
		dd->pdata->pmic_backlight(dd->pdata->shrink_pwm(bl_level));
		
	bl_level_prevset = mfd->bl_level;
}

static int __devinit auo_probe(struct platform_device *pdev)
{
	int rc = 0;

	if (pdev->id == 0) {
		dd = kzalloc(sizeof *dd, GFP_KERNEL);
		if (!dd)
			return -ENOMEM;

		dd->pdata = pdev->dev.platform_data;
		return 0;
	} else if (!dd)
		return -ENODEV;
#if 0
#ifdef CONFIG_PMIC8058_PWM
	bl_pwm0 = pwm_request(dd->pdata->gpio_num[0], "backlight1");
	if (bl_pwm0 == NULL || IS_ERR(bl_pwm0)) {
		pr_err("%s pwm_request() failed\n", __func__);
		bl_pwm0 = NULL;
	}

	bl_pwm1 = pwm_request(dd->pdata->gpio_num[1], "backlight2");
	if (bl_pwm1 == NULL || IS_ERR(bl_pwm1)) {
		pr_err("%s pwm_request() failed\n", __func__);
		bl_pwm1 = NULL;
	}

	printk(KERN_INFO "auo_probe: bl_pwm0=%p LPG_chan0=%d "
			"bl_pwm1=%p LPG_chan1=%d\n",
			bl_pwm0, (int)dd->pdata->gpio_num[0],
			bl_pwm1, (int)dd->pdata->gpio_num[1]
			);
#endif
#endif

	dd->fbpdev = msm_fb_add_device(pdev);
	if (!dd->fbpdev) {
		dev_err(&pdev->dev, "failed to add msm_fb device\n");
		rc = -ENODEV;
		goto probe_exit;
	}


probe_exit:
	return rc;
}


static struct platform_driver this_driver = {
	.probe  = auo_probe,
	.driver = {
		.name   = "lcdc_auo_wxga",
	},
};

static struct msm_fb_panel_data auo_panel_data = {
	.set_backlight = lcdc_auo_panel_set_backlight,
	.bklswitch	= lcdc_auo_panel_bkl_switch,
	.bklctrl	= lcdc_auo_panel_bkl_ctrl,
};

static struct platform_device this_device = {
	.name   = "lcdc_auo_wxga",
	.id	= 1,
	.dev	= {
		.platform_data = &auo_panel_data,
	}
};

static int __init lcdc_auo_panel_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

#ifdef CONFIG_FB_MSM_MIPI_PANEL_DETECT
	if (msm_fb_detect_client("lcdc_auo_wxga"))
		return 0;
#endif

	ret = platform_driver_register(&this_driver);
	if (ret)
		return ret;

	pinfo = &auo_panel_data.panel_info;

	pinfo->xres = 1280;
	pinfo->yres = 800;
	MSM_FB_SINGLE_MODE_PANEL(pinfo);
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	if (dd && dd->pdata)
		pinfo->bpp = dd->pdata->rgb_format();
	pinfo->fb_num = 2;
	pinfo->clk_rate = 71100000;
	pinfo->bl_max = 255;
	pinfo->bl_min = 1;

	pinfo->lcdc.h_back_porch = 90;
	pinfo->lcdc.h_front_porch = 64;
	pinfo->lcdc.h_pulse_width = 32;
	pinfo->lcdc.v_back_porch = 4;
	pinfo->lcdc.v_front_porch = 3;
	pinfo->lcdc.v_pulse_width = 1;
	pinfo->lcdc.border_clr = 0;
	pinfo->lcdc.underflow_clr = 0xff;
	pinfo->lcdc.hsync_skew = 0;

	ret = platform_device_register(&this_device);
	if (ret)
		platform_driver_unregister(&this_driver);

	return ret;
}

module_init(lcdc_auo_panel_init);
