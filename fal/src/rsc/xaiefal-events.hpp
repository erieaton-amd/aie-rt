// (c) Copyright(C) 2020 - 2021 by Xilinx, Inc. All rights reserved.
// SPDX-License-Identifier: MIT

#include <fstream>
#include <functional>
#include <string.h>
#include <vector>
#include <xaiengine.h>

#include <xaiefal/rsc/xaiefal-rsc-base.hpp>

#pragma once

namespace xaiefal {
	/**
	 * @class XAieComboEvent
	 * @brief AI engine combo event resource class
	 */
	class XAieComboEvent: public XAieSingleTileRsc {
	public:
		XAieComboEvent() = delete;
		XAieComboEvent(std::shared_ptr<XAieDevHandle> DevHd,
			XAie_LocType L, XAie_ModuleType M, uint32_t ENum = 2):
			XAieSingleTileRsc(DevHd, L, M) {
			if (ENum > 4 || ENum < 2) {
				throw std::invalid_argument("Combo event failed, invalid input events number");
			}
			vEvents.resize(ENum);
			State.Initialized = 1;
		}
		XAieComboEvent(XAieDev &Dev,
			XAie_LocType L, XAie_ModuleType M, uint32_t ENum = 2):
			XAieComboEvent(Dev.getDevHandle(), L, M, ENum) {}
		/**
		 * This function sets input events, and combo operations.
		 *
		 * @param vE vector of input events.Minum 2 events, maximum 4 events
		 *	vE[0] for Event0, vE[1] for Event1,
		 *	vE[2] for Event2, vE[3] for Event3
		 * @param vOp vector of combo operations
		 *	vOp[0] for combo operation for Event0 and Event1
		 *	vOp[1] for combo operation for Event2 and Event3
		 *	vOp[2] for combo operation for (Event0,Event1) and
		 *		(Event2,Event3)
		 * @return XAIE_OK for success, error code for failure.
		 */
		AieRC setEvents(const std::vector<XAie_Events> &vE,
				const std::vector<XAie_EventComboOps> &vOp) {
			AieRC RC;

			if (State.Initialized == 0) {
				Logger::log(LogLevel::ERROR) << "combo event " << __func__ << " (" <<
					(uint32_t)Loc.Col << "," << (uint32_t)Loc.Row <<
					" Mod=" << Mod <<  " not initialized with Mod and num of events." << std::endl;
				RC = XAIE_ERR;
			} else if ((vE.size() != vEvents.size()) || (vOp.size() > 3) ||
			    (vE.size() <= 2 && vOp.size() > 1) ||
			    (vE.size() > 2 && vOps.size() < 2)) {
				Logger::log(LogLevel::ERROR) << "combo event " << __func__ << " (" <<
					(uint32_t)Loc.Col << "," << (uint32_t)Loc.Row <<
					" Mod=" << Mod <<  " invalid number of input events and ops." << std::endl;
				RC = XAIE_INVALID_ARGS;
			} else {
				for (int i = 0; i < (int)vE.size(); i++) {
					uint8_t HwEvent;

					RC = XAie_EventLogicalToPhysicalConv(dev(), Loc,
							Mod, vE[i], &HwEvent);
					if (RC != XAIE_OK) {
						Logger::log(LogLevel::ERROR) << "combo event " << __func__ << " (" <<
							(uint32_t)Loc.Col << "," << (uint32_t)Loc.Row <<
							" Mod=" << Mod <<  " invalid E=" << vE[i] << std::endl;
						break;
					} else {
						vEvents[i] = vE[i];
					}
				}
				if (RC == XAIE_OK) {
					vOps.clear();
					for (int i = 0; i < (int)vOp.size(); i++) {
						vOps.push_back(vOp[i]);
					}
					State.Configured = 1;
				}
			}
			return RC;
		}
		/**
		 * This function returns combo events for the input combination.
		 *
		 * @param vE combo events vector
		 *	vE[0] for combination of input events Event0, Event1
		 *	vE[1] for combination of input events Event2, Event3
		 *	vE[2] for combination of input events (Event0,Event1)
		 *		and (Event2,Event3)
		 * @return XAIE_OK for success, error code for failure.
		 */
		AieRC getEvents(std::vector<XAie_Events> &vE) {
			AieRC RC;

			(void)vE;
			if (State.Reserved == 0) {
				Logger::log(LogLevel::ERROR) << "combo event " << __func__ << " (" <<
					(uint32_t)Loc.Col << "," << (uint32_t)Loc.Row <<
					" Mod=" << Mod <<  " resource is not reserved." << std::endl;
				RC = XAIE_ERR;
			} else {
				XAie_Events BaseE;
				uint32_t TType = _XAie_GetTileTypefromLoc(dev(), Loc);

				if (TType == XAIEGBL_TILE_TYPE_AIETILE) {
					if (Mod == XAIE_CORE_MOD) {
						BaseE = XAIE_EVENT_COMBO_EVENT_0_CORE;
					} else {
						BaseE = XAIE_EVENT_COMBO_EVENT_0_MEM;
					}
				} else {
					BaseE = XAIE_EVENT_COMBO_EVENT_0_PL;
				}
				vE.clear();
				if (vOps.size() == 1) {
					vE.push_back((XAie_Events)((uint32_t)BaseE + vRscs[0].RscId));
				} else {
					for (uint32_t i = 0; i < (uint32_t)vOps.size(); i++) {
						vE.push_back((XAie_Events)((uint32_t)BaseE + i));
					}
				}
				RC = XAIE_OK;
			}
			return RC;
		}

		AieRC getInputEvents(std::vector<XAie_Events> &vE,
				std::vector<XAie_EventComboOps> &vOp) {
			AieRC RC;

			if (State.Configured == 1) {
				vE.clear();
				for (int i = 0; i < (int)vEvents.size(); i++) {
					vE.push_back(vEvents[i]);
				}
				vOp.clear();
				for (int i = 0; i < (int)vOps.size(); i++) {
					vOp.push_back(vOps[i]);
				}
				RC = XAIE_OK;
			} else {
				Logger::log(LogLevel::ERROR) << "combo event " << __func__ << " (" <<
					(uint32_t)Loc.Col << "," << (uint32_t)Loc.Row <<
					" Mod=" << Mod <<  " no input events specified." << std::endl;
				RC = XAIE_ERR;
			}
			return RC;
		}
	protected:
		std::vector<XAie_Events> vEvents; /**< input events */
		std::vector<XAie_EventComboOps> vOps; /**< combo operations */
		std::vector<XAie_UserRsc> vRscs; /**< combo events resources */
	private:
		AieRC _reserve() {
			AieRC RC;

			// TODO: reserve from c driver
			for (int i = 0; i < (int)vEvents.size(); i++) {
				XAie_UserRsc R;

				RC = XAieComboEvent::XAieAllocRsc(AieHd, Loc, Mod, R);
				if (RC != XAIE_OK) {
					for (int j = 0; j < i; j++) {
						XAieComboEvent::XAieReleaseRsc(AieHd, vRscs[j]);
					}
				} else {
					vRscs.push_back(R);
				}
			}
			Rsc.Mod = vRscs[0].Mod;
			if (vRscs.size() <= 2) {
				// Only two input events, it can be combo0 or
				// combo1
				if (vRscs[0].RscId < 2) {
					Rsc.RscId = 0;
				} else {
					Rsc.RscId = 1;
				}
			} else {
				Rsc.RscId = 2;
			}
			return XAIE_OK;
		}
		AieRC _release() {
			// TODO: release from c driver
			for (auto r: vRscs) {
				XAieComboEvent::XAieReleaseRsc(AieHd, r);
			}
			vRscs.clear();
			return XAIE_OK;
		}
		AieRC _start() {
			AieRC RC;
			XAie_EventComboId StartCId;

			for (int i = 0 ; i < (int)vEvents.size(); i += 2) {
				XAie_EventComboId ComboId;

				if (vRscs[i].RscId == 0) {
					ComboId = XAIE_EVENT_COMBO0;
				} else {
					ComboId = XAIE_EVENT_COMBO1;
				}
				if (i == 0) {
					StartCId = ComboId;
				}
				RC = XAie_EventComboConfig(dev(), Loc, Mod,
						ComboId, vOps[i/2], vEvents[i], vEvents[i+1]);
				if (RC != XAIE_OK) {
					Logger::log(LogLevel::ERROR) << "combo event " << __func__ << " (" <<
						(uint32_t)Loc.Col << "," << (uint32_t)Loc.Row <<
						" Mod=" << Mod <<  " failed to config combo " << ComboId << std::endl;
					for (XAie_EventComboId tId = StartCId;
						tId < ComboId;
						tId = (XAie_EventComboId)((int)tId + i)) {
						XAie_EventComboReset(dev(), Loc, Mod,
								tId);
					}
					break;
				}
			}
			if (RC == XAIE_OK && vOps.size() == 3) {
				RC = XAie_EventComboConfig(dev(), Loc, Mod,
					XAIE_EVENT_COMBO2, vOps[2], vEvents[0], vEvents[0]);
				if (RC != XAIE_OK) {
					Logger::log(LogLevel::ERROR) << "combo event " << __func__ << " (" <<
						(uint32_t)Loc.Col << "," << (uint32_t)Loc.Row <<
						" Mod=" << Mod <<  " failed to config combo " << XAIE_EVENT_COMBO2 << std::endl;
				}
			}
			return RC;
		}
		AieRC _stop() {
			XAie_EventComboId ComboId;

			for (int i = 0; i < (int)vOps.size(); i++) {
				if (i == 0) {
					if (vRscs[i].RscId == 0) {
						ComboId = XAIE_EVENT_COMBO0;
					} else {
						ComboId = XAIE_EVENT_COMBO1;
					}
					XAie_EventComboReset(dev(), Loc, Mod, ComboId);
					ComboId = (XAie_EventComboId)((uint32_t)ComboId + 1);
				}
			}
			return XAIE_OK;
		}
	private:
		/**
		 * TODO: Following function will not be required.
		 * Bitmap will be moved to c device driver
		 */
		static AieRC XAieAllocRsc(std::shared_ptr<XAieDevHandle> Dev, XAie_LocType L,
				XAie_ModuleType M, XAie_UserRsc &R) {
			uint64_t *bits;
			int sbit;
			AieRC RC = XAIE_OK;

			(void)Dev;
			if ((R.Loc.Row == 0 && R.Mod != XAIE_PL_MOD) ||
			    (R.Loc.Row != 0 && R.Mod == XAIE_PL_MOD)) {
				Logger::log(LogLevel::ERROR) << __func__ <<
					"Combo: invalid tile and module." << std::endl;
				return XAIE_INVALID_ARGS;
			}
			if (M == XAIE_CORE_MOD) {
				bits = Dev->XAieComboCoreBits;
				sbit = (L.Col * 8 + (L.Row - 1)) * 4;
			} else if (M == XAIE_MEM_MOD) {
				bits = Dev->XAieComboMemBits;
				sbit = (L.Col * 8 + (L.Row - 1)) * 4;
			} else if (M == XAIE_PL_MOD) {
				bits = Dev->XAieComboShimBits;
				sbit = L.Col * 4;
			} else {
				Logger::log(LogLevel::ERROR) << __func__ <<
					"invalid module type" << std::endl;
				RC = XAIE_INVALID_ARGS;
			}
			if (RC == XAIE_OK) {
				int bits2check = 4;
				int bit = XAieRsc::alloc_rsc_bit(bits, sbit, bits2check);

				if (bit < 0) {
					RC = XAIE_ERR;
				} else {
					R.Loc = L;
					R.Mod = M;
					R.RscId = bit - sbit;
				}
			}
			return RC;
		}
		/**
		 * TODO: Following function will not be required.
		 * Bitmap will be moved to device driver
		 */
		static void XAieReleaseRsc(std::shared_ptr<XAieDevHandle> Dev,
				const XAie_UserRsc &R) {
			uint64_t *bits;
			int pos;

			(void)Dev;
			if ((R.Loc.Row == 0 && R.Mod != XAIE_PL_MOD) ||
			    (R.Loc.Row != 0 && R.Mod == XAIE_PL_MOD)) {
				Logger::log(LogLevel::ERROR) << __func__ <<
					"PCount: invalid tile and module." << std::endl;
				return;
			} else if (R.Mod == XAIE_PL_MOD) {
				bits = Dev->XAieComboShimBits;
				pos = R.Loc.Col * 4 + R.RscId;
			} else if (R.Mod == XAIE_CORE_MOD) {
				bits = Dev->XAieComboCoreBits;
				pos = (R.Loc.Col * 8 + (R.Loc.Row - 1)) * 4 + R.RscId;
			} else {
				bits = Dev->XAieComboMemBits;
				pos = (R.Loc.Col * 8 + (R.Loc.Row - 1)) * 4 + R.RscId;
			}
			XAieRsc::clear_rsc_bit(bits, pos);
		}
	};
}