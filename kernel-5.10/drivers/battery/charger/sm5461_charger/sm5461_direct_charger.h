/*
 * sm5461_direct_charger.h - Direct charging module for Silicon Mitus ICs
 *
 * Copyright (C) 2023 Silicon Mitus Co.Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/mutex.h>

#ifndef __SM_DC_H__
#define __SM_DC_H__

#define SM_DC_BYPASS_TA_MAX_VOL	7000
#define SM_DC_MANUAL_TA_MAX_CUR	3000
#define SM_DC_DUAL_STOP_IBUS    1500
#define SM_DC_CI_OFFSET_X2BAT   500

#define PPS_V_STEP              20
#define PPS_C_STEP              50

#define PRE_CC_ST_IBUS_OFFSET   150
#define CC_ST_IBUS_OFFSET       100
#define CV_ST_SUB_DC_OFF_IBUS   1000

#define MAX(a, b)               ((a > b) ? (a):(b))
#define MIN(a, b)               ((a < b) ? (a):(b))

enum sm_dc_charging_loop {
	LOOP_IBUSREG                = (0x1 << 7),
	LOOP_IBUSREG_M              = (0x1 << 6),
	LOOP_IBATREG                = (0x1 << 5),
	LOOP_VBATREG                = (0x1 << 3),
	LOOP_VBATREG_S              = (0x1 << 2),
	LOOP_THEMREG                = (0x1 << 1),
	LOOP_INACTIVE               = (0x0),
};

enum sm_dc_work_delay_type {
	DELAY_NONE                  = 0,
	DELAY_PPS_UPDATE            = 250,
	DELAY_ADC_UPDATE            = 1100,
	DELAY_RETRY                 = 2000,
	DELAY_CHG_LOOP              = 2500,
};

enum sm_dc_power_supply_type {
	SM_DC_POWER_SUPPLY_PD       = 0x0,
	SM_DC_POWER_SUPPLY_2XBAT    = 0x1,
};

enum sm_dc_state {
	/* SEC_DIRECT_CHG_MODE_DIRECT_OFF */
	SM_DC_CHG_OFF       = 0x0,
	SM_DC_ERR,
	/* SEC_DIRECT_CHG_MODE_DIRECT_DONE */
	SM_DC_EOC,
	/* SEC_DIRECT_CHG_MODE_DIRECT_CHECK_VBAT */
	SM_DC_CHECK_VBAT,
	/* SEC_DIRECT_CHG_MODE_DIRECT_PRESET */
	SM_DC_PRESET,
	/* SEC_DIRECT_CHG_MODE_DIRECT_ON_ADJUST */
	SM_DC_PRE_CC,
	SM_DC_UPDAT_BAT,
	/* SEC_DIRECT_CHG_MODE_DIRECT_ON */
	SM_DC_CC,
	SM_DC_CV,
	SM_DC_CV_MAN,
	SM_DC_CV_FPDO,
};

enum sm_dc_err_index {
	SM_DC_ERR_NONE              = (0x0),
	SM_DC_ERR_VBATREG           = (0x1 << 0),
	SM_DC_ERR_IBUSREG           = (0x1 << 1),
	SM_DC_ERR_TSD               = (0x1 << 2),
	SM_DC_ERR_VBATOVP           = (0x1 << 3),
	SM_DC_ERR_VOUTOVP           = (0x1 << 4),
	SM_DC_ERR_IBUSUCP           = (0x1 << 5),
	SM_DC_ERR_IBATOCP           = (0x1 << 6),
	SM_DC_ERR_IBUSOCP           = (0x1 << 7),
	SM_DC_ERR_CFLY_SHORT        = (0x1 << 8),
	SM_DC_ERR_REVBLK            = (0x1 << 9),
	SM_DC_ERR_STUP_FAIL         = (0x1 << 10),
	SM_DC_ERR_CN_SHORT          = (0x1 << 11),
	SM_DC_ERR_IBUSREG_M         = (0x1 << 12),
	SM_DC_ERR_VBATREG_S         = (0x1 << 13),
	SM_DC_ERR_VBUSUVLO          = (0x1 << 14),
	SM_DC_ERR_VBUSOVP           = (0x1 << 15),
	SM_DC_ERR_INVAL_VBAT        = (0x1 << 16),
	SM_DC_ERR_SEND_PD_MSG       = (0x1 << 17),
	SM_DC_ERR_FAIL_ADJUST       = (0x1 << 18),
	SM_DC_ERR_RETRY             = (0x1 << 30),
	SM_DC_ERR_UNKNOWN           = (0x1 << 31),
};

enum sm_dc_interrupt_index {
	SM_DC_INT_VBATREG           = (0x1 << 0),
	SM_DC_INT_WDTOFF            = (0x1 << 1),
};

enum sm_dc_adc_channel {
	SM_DC_ADC_THEM         = 0x0,
	SM_DC_ADC_DIETEMP,
	SM_DC_ADC_VBAT,
	SM_DC_ADC_IBAT,
	SM_DC_ADC_VOUT,
	SM_DC_ADC_VBUS,
	SM_DC_ADC_IBUS,
};

enum sm_dc_adc_mode {
	SM_DC_ADC_MODE_ONESHOT     = 0x0,
	SM_DC_ADC_MODE_CONTINUOUS  = 0x1,
	SM_DC_ADC_MODE_OFF         = 0x2,
};

struct sm_dc_power_source_info {
	u32 pdo_pos;
	u32 v_max;
	u32 c_max;
	u32 p_max;
	u32 v;
	u32 c;
	u32 retry_cnt;
};

struct sm_dc_ops {
	int (*get_adc_value)(struct i2c_client *i2c, u8 adc_ch);
	int (*set_adc_mode)(struct i2c_client *i2c, u8 mode);
	int (*get_charging_enable)(struct i2c_client *i2c);
	int (*set_charging_enable)(struct i2c_client *i2c, bool enable);
	int (*set_charging_config)(struct i2c_client *i2c, u32 cv_gl, u32 ci_gl, u32 cc_gl);
	u32 (*get_dc_error_status)(struct i2c_client *i2c);
	int (*send_power_source_msg)(struct i2c_client *i2c, struct sm_dc_power_source_info *ta);
	int (*get_apdo_max_power)(struct i2c_client *i2c, struct sm_dc_power_source_info *ta);
	u32 (*get_target_ibus)(struct i2c_client *i2c);
	int (*check_sw_ocp)(struct i2c_client *i2c);
};

struct sm_dc_info {
	const char *name;
	struct i2c_client *i2c;
	struct i2c_client *i2c_sub;
	struct mutex st_lock;
	const struct sm_dc_ops *ops;
	int chip_id;

	/* for direct-charging state machine */
	struct workqueue_struct *dc_wqueue;
	struct delayed_work check_vbat_work;
	struct delayed_work preset_dc_work;
	struct delayed_work pre_cc_work;
	struct delayed_work cc_work;
	struct delayed_work cv_work;
	struct delayed_work cv_man_work;
	struct delayed_work cv_fpdo_work;
	struct delayed_work update_bat_work;
	struct delayed_work error_work;

	/* for SEC-BATTERY done event process */
	struct delayed_work done_event_work;

	u8 state;
	u32 err;
	bool req_update_vbat;
	bool req_update_ibus;
	bool req_update_ibat;
	u32 target_vbat;
	u32 target_ibat;
	u32 target_ibus;

	struct sm_dc_power_source_info ta;

	/* for state machine control */
	struct {
		bool pps_cl;
		bool c_up;
		bool c_down;
		bool v_up;
		bool v_down;
		bool pps_vcm;
		bool topoff_m;
		bool topoff_s;
		int v_offset;
		int c_offset;
		u16 prev_adc_ibus;
		u16 prev_adc_vbus;
		bool cc_limit;
		int cc_cnt;
		int cv_cnt;
		u32 cv_gl;
		u32 ci_gl;
		u32 ci_gl_m;
		u32 ci_gl_s;
		u32 cc_gl;
		u32 target_pps_v;
		int retry_cnt;
		int vbatreg_cnt;
	} wq;

	struct {
		u32 ta_min_current;
		u32 ta_min_voltage;
		u32 dc_min_vbat;
		u32 dc_vbus_ovp_th;
		u32 pps_lr;
		u32 rpara;
		u32 rsns;
		u32 rpcm;
		u32 r_ttl;
		u32 topoff_current;
		bool need_to_sw_ocp;
		bool support_pd_remain;
		/* sec_battery info */
		u32 chg_float_voltage;
		u32 vbus_ctrl_mode;
		u32 fpdo_topoff;
		u32 fpdo_mainvbat_reg;
		u32 fpdo_subvbat_reg;
		u32 fpdo_vnow_reg;
		u32 fpdo_chg_curr;
		char *sec_dc_name;
	} config;
};

extern struct sm_dc_info *sm_dc_create_pd_instance(const char *name, struct i2c_client *i2c);
extern struct sm_dc_info *sm_dc_create_x2bat_instance(const char *name, struct i2c_client *i2c);
extern int sm_dc_verify_configuration(struct sm_dc_info *sm_dc);
extern void sm_dc_destroy_instance(struct sm_dc_info *sm_dc);

extern int sm_dc_report_error_status(struct sm_dc_info *sm_dc, u32 err);
extern int sm_dc_report_interrupt_event(struct sm_dc_info *sm_dc, u32 interrupt);

extern int sm_dc_get_current_state(struct sm_dc_info *sm_dc);
extern int sm_dc_start_charging(struct sm_dc_info *sm_dc);
extern int sm_dc_stop_charging(struct sm_dc_info *sm_dc);
extern int sm_dc_start_manual_charging(struct sm_dc_info *sm_dc);
extern int sm_dc_start_fpdo_charging(struct sm_dc_info *sm_dc);
extern int sm_dc_set_ta_volt_by_soc(struct sm_dc_info *sm_dc, int delta_soc);
extern int sm_dc_set_target_vbat(struct sm_dc_info *sm_dc, u32 target_vbat);
extern int sm_dc_set_target_ibus(struct sm_dc_info *sm_dc, u32 target_ibus);

#endif /* __SM_DC_H__ */
