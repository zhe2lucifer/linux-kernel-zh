/*------------------------------------------------------------------------------
  Copyright 2016 Sony Semiconductor Solutions Corporation

  Last Updated    : 2016/08/01
  Modification ID : b30d76210d343216ea52b88e9b450c8fd5c0359f
------------------------------------------------------------------------------*/
#include "sony_integ.h"
#include "sony_integ_j83b.h"
#include "sony_demod.h"
#include "sony_demod_j83b.h"
#include "sony_demod_j83b_monitor.h"

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/
sony_result_t sony_integ_j83b_Tune (sony_integ_t * pInteg,
                                    sony_j83b_tune_param_t * pTuneParam)
{
    sony_result_t result = SONY_RESULT_OK;

    SONY_TRACE_ENTER ("sony_integ_j83b_Tune");

    if ((!pInteg) || (!pTuneParam) || (!pInteg->pDemod)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if ((pInteg->pDemod->state != SONY_DEMOD_STATE_SLEEP) && (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE)) {
        /* This api is accepted in Sleep and Active states only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    if (!pInteg->pDemod->scanMode) {
        /* Clear cancellation flag. */
        sony_atomic_set (&(pInteg->cancel), 0);
    }

    /* Check bandwidth validity */
    if ((SONY_DTV_BW_J83B_5_06_5_36_MSPS != pTuneParam->bandwidth) &&
        (SONY_DTV_BW_J83B_5_60_MSPS != pTuneParam->bandwidth)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_NOSUPPORT);
    }

    /* Confirm Tune parameters */
    if (0 == pTuneParam->centerFreqKHz) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Tune the demodulator */
    result = sony_demod_j83b_Tune (pInteg->pDemod, pTuneParam);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    if ((pInteg->pTunerTerrCable) && (pInteg->pTunerTerrCable->Tune)) {
        /* Enable the I2C repeater */
        result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x01);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }

        /* Tune the RF part */
        result = pInteg->pTunerTerrCable->Tune (pInteg->pTunerTerrCable, pTuneParam->centerFreqKHz, SONY_DTV_SYSTEM_J83B, pTuneParam->bandwidth);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }

        /* Disable the I2C repeater */
        result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x00);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
    }

    /* Reset the demod to enable acquisition */
    result = sony_demod_TuneEnd (pInteg->pDemod);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    /* Wait for demodulator (TS) lock */
    result = sony_integ_j83b_WaitTSLock (pInteg);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    SONY_TRACE_RETURN (result);
}

sony_result_t sony_integ_j83b_Scan (sony_integ_t * pInteg,
                                    sony_integ_j83b_scan_param_t * pScanParam,
                                    sony_integ_j83b_scan_callback_t callBack)
{
    sony_result_t result = SONY_RESULT_OK;
    sony_integ_j83b_scan_result_t scanResult;

    SONY_TRACE_ENTER ("sony_integ_j83b_Scan");

    if ((!pInteg) || (!pScanParam) || (!callBack) || (!pInteg->pDemod)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if ((pInteg->pDemod->state != SONY_DEMOD_STATE_SLEEP) && (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE)) {
        /* This api is accepted in Sleep and Active states only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    /* Clear cancellation flag. */
    sony_atomic_set (&(pInteg->cancel), 0);

    /* Check bandwidth validity */
    if ((SONY_DTV_BW_J83B_5_06_5_36_MSPS != pScanParam->bandwidth) &&
        (SONY_DTV_BW_J83B_5_60_MSPS != pScanParam->bandwidth)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_NOSUPPORT);
    }

    /* Ensure the scan parameters are valid. */
    if (pScanParam->endFrequencyKHz < pScanParam->startFrequencyKHz) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (pScanParam->stepFrequencyKHz == 0) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Enable scan mode */
    result = sony_demod_terr_cable_SetScanMode (pInteg->pDemod, SONY_DTV_SYSTEM_J83B, 0x01);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    /* Set the start frequency */
    scanResult.centerFreqKHz = pScanParam->startFrequencyKHz;
    scanResult.tuneParam.bandwidth = pScanParam->bandwidth;

    /* Scan routine */
    while (scanResult.centerFreqKHz <= pScanParam->endFrequencyKHz) {
        sony_j83b_tune_param_t tuneParam;

        tuneParam.centerFreqKHz = scanResult.centerFreqKHz;
        tuneParam.bandwidth = scanResult.tuneParam.bandwidth;

        scanResult.tuneResult = sony_integ_j83b_Tune (pInteg, &tuneParam);
        switch (scanResult.tuneResult) {
        case SONY_RESULT_OK:
            /* Channel found, callback to application */
            scanResult.tuneParam = tuneParam;
            scanResult.centerFreqKHz = tuneParam.centerFreqKHz;
            callBack (pInteg, &scanResult, pScanParam);
            break;

        /* Intentional fall-through. */
        case SONY_RESULT_ERROR_UNLOCK:
        case SONY_RESULT_ERROR_TIMEOUT:
            /* Channel not found, callback to applicaiton for progress updates */
            callBack (pInteg, &scanResult, pScanParam);
            break;

        default:
            /* Serious error occurred -> cancel operation. */
            sony_demod_terr_cable_SetScanMode (pInteg->pDemod, SONY_DTV_SYSTEM_J83B, 0x00);
            SONY_TRACE_RETURN (scanResult.tuneResult);
        }

        scanResult.centerFreqKHz += pScanParam->stepFrequencyKHz;

        /* Check cancellation. */
        result = sony_integ_CheckCancellation (pInteg);
        if (result != SONY_RESULT_OK) {
            sony_demod_terr_cable_SetScanMode (pInteg->pDemod, SONY_DTV_SYSTEM_J83B, 0x00);
            SONY_TRACE_RETURN (result);
        }
    }

    /* Clear scan mode */
    result = sony_demod_terr_cable_SetScanMode (pInteg->pDemod, SONY_DTV_SYSTEM_J83B, 0x00);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    SONY_TRACE_RETURN (result);
}

sony_result_t sony_integ_j83b_WaitTSLock (sony_integ_t * pInteg)
{
    sony_result_t result = SONY_RESULT_OK;
    sony_demod_lock_result_t lock = SONY_DEMOD_LOCK_RESULT_NOTDETECT;
    sony_stopwatch_t timer;
    uint8_t continueWait = 1;
    uint32_t elapsed = 0;

    SONY_TRACE_ENTER ("sony_integ_j83b_WaitTSLock");

    if ((!pInteg) || (!pInteg->pDemod)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in Active state only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    /* Wait for demod lock */
    result = sony_stopwatch_start (&timer);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    for (;;) {
        result = sony_stopwatch_elapsed(&timer, &elapsed);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }

        if (elapsed >= SONY_J83B_WAIT_DEMOD_LOCK) {
            continueWait = 0;
        }

        result = sony_demod_j83b_CheckTSLock (pInteg->pDemod, &lock);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }

        switch (lock) {
        case SONY_DEMOD_LOCK_RESULT_LOCKED:
            SONY_TRACE_RETURN (SONY_RESULT_OK);

        case SONY_DEMOD_LOCK_RESULT_UNLOCKED:
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_UNLOCK);

        default:
            /* continue waiting... */
            break;
        }

        /* Check cancellation. */
        result = sony_integ_CheckCancellation (pInteg);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }

        if (continueWait) {
            result = sony_stopwatch_sleep (&timer, SONY_J83B_WAIT_LOCK_INTERVAL);
            if (result != SONY_RESULT_OK) {
                SONY_TRACE_RETURN (result);
            }
        }
        else {
            result = SONY_RESULT_ERROR_TIMEOUT;
            break;
        }
    }

    SONY_TRACE_RETURN (result);
}

sony_result_t sony_integ_j83b_monitor_RFLevel (sony_integ_t * pInteg, int32_t * pRFLeveldB)
{
    sony_result_t result = SONY_RESULT_OK;

    SONY_TRACE_ENTER ("sony_integ_j83b_monitor_RFLevel");

    if ((!pInteg) || (!pInteg->pDemod) || (!pRFLeveldB)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE) {
        /* This api is accepted in Active state only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    if (pInteg->pDemod->system != SONY_DTV_SYSTEM_J83B) {
        /* Not J.83B */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    if (!pInteg->pTunerTerrCable) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_NOSUPPORT);
    }

    if (pInteg->pTunerTerrCable->ReadRFLevel) {
        /* Enable the I2C repeater */
        result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x01);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }

        result = pInteg->pTunerTerrCable->ReadRFLevel (pInteg->pTunerTerrCable, pRFLeveldB);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }

        /* Disable the I2C repeater */
        result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x00);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
    } else if (pInteg->pTunerTerrCable->CalcRFLevelFromAGC) {
        uint32_t ifAgc;

        result = sony_demod_j83b_monitor_IFAGCOut(pInteg->pDemod, &ifAgc);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }

        result = pInteg->pTunerTerrCable->CalcRFLevelFromAGC (pInteg->pTunerTerrCable, ifAgc, pRFLeveldB);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
    } else {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_NOSUPPORT);
    }

    SONY_TRACE_RETURN (SONY_RESULT_OK);
}
