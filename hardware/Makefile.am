##
## This file is part of the sigrok project.
##
## Copyright (C) 2011 Uwe Hermann <uwe@hermann-uwe.de>
## Copyright (C) 2012 Alexandru Gagniuc <mr.nuke.me@gmail.com>
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

SUBDIRS = \
	agilent-dmm \
	alsa \
	asix-sigma \
	brymen-dmm \
	chronovu-la8 \
	colead-slm \
	common \
	demo \
	fluke-dmm \
	fx2lafw \
	hantek-dso \
	lascar-el-usb \
	link-mso19 \
	mic-985xx \
	nexus-osciprime \
	openbench-logic-sniffer \
	rigol-ds1xx2 \
	serial-dmm \
	tondaj-sl-814 \
	uni-t-dmm \
	victor-dmm \
	zeroplus-logic-cube

noinst_LTLIBRARIES = libsigrokhardware.la

libsigrokhardware_la_SOURCES =

libsigrokhardware_la_LIBADD = \
	common/libsigrokhwcommon.la

if HW_AGILENT_DMM
libsigrokhardware_la_LIBADD += agilent-dmm/libsigrokhwagilentdmm.la
endif

if HW_ALSA
libsigrokhardware_la_LIBADD += alsa/libsigrokhwalsa.la
endif

if LA_ASIX_SIGMA
libsigrokhardware_la_LIBADD += asix-sigma/libsigrokhwasixsigma.la
endif

if HW_BRYMEN_DMM
libsigrokhardware_la_LIBADD += brymen-dmm/libsigrok_hw_brymen_dmm.la
endif

if LA_CHRONOVU_LA8
libsigrokhardware_la_LIBADD += chronovu-la8/libsigrokhwchronovula8.la
endif

if HW_COLEAD_SLM
libsigrokhardware_la_LIBADD += colead-slm/libsigrok_hw_colead_slm.la
endif

if LA_DEMO
libsigrokhardware_la_LIBADD += demo/libsigrokhwdemo.la
endif

if HW_FLUKE_DMM
libsigrokhardware_la_LIBADD += fluke-dmm/libsigrokhwflukedmm.la
endif

if LA_FX2LAFW
libsigrokhardware_la_LIBADD += fx2lafw/libsigrokhwfx2lafw.la
endif

if HW_HANTEK_DSO
libsigrokhardware_la_LIBADD += hantek-dso/libsigrokhw_hantek_dso.la
endif

if HW_LASCAR_EL_USB
libsigrokhardware_la_LIBADD += lascar-el-usb/libsigrok_hw_lascar_el_usb.la
endif

if LA_LINK_MSO19
libsigrokhardware_la_LIBADD += link-mso19/libsigrokhwlinkmso19.la
endif

if HW_MIC_985XX
libsigrokhardware_la_LIBADD += mic-985xx/libsigrok_hw_mic_985xx.la
endif

if HW_NEXUS_OSCIPRIME
libsigrokhardware_la_LIBADD += nexus-osciprime/libsigrok_hw_nexus_osciprime.la
endif

if LA_OLS
libsigrokhardware_la_LIBADD += openbench-logic-sniffer/libsigrokhwols.la
endif

if HW_RIGOL_DS1XX2
libsigrokhardware_la_LIBADD += rigol-ds1xx2/libsigrok_hw_rigol_ds1xx2.la
endif

if HW_SERIAL_DMM
libsigrokhardware_la_LIBADD += serial-dmm/libsigrokhwserialdmm.la
endif

if HW_TONDAJ_SL_814
libsigrokhardware_la_LIBADD += tondaj-sl-814/libsigrok_hw_tondaj_sl_814.la
endif

if HW_UNI_T_DMM
libsigrokhardware_la_LIBADD += uni-t-dmm/libsigrok_hw_uni_t_dmm.la
endif

if HW_VICTOR_DMM
libsigrokhardware_la_LIBADD += victor-dmm/libsigrok_hw_victor_dmm.la
endif

if LA_ZEROPLUS_LOGIC_CUBE
libsigrokhardware_la_LIBADD += zeroplus-logic-cube/libsigrokhwzeroplus.la
endif

