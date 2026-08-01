/*
 * SUMMARY:      MassEnergyBalance.c - Calculate mass and energy balance
 * USAGE:        Part of DHSVM
 *
 * AUTHOR:       Bart Nijssen
 * ORG:          University of Washington, Department of Civil Engineering
 * E-MAIL:       nijssen@u.washington.edu
 * ORIG-DATE:    Apr-96
 * DESCRIPTION:  Calculate mass and energy balance at each pixel
 * DESCRIP-END.
 * FUNCTIONS:    MassEnergyBalance()
 * COMMENTS:
 * $Id: MassEnergyBalance.c,v3.1.2 2013/08/18 ning Exp $
 */
#ifdef SNOW_ONLY
  //#define NO_ET
  #define NO_SOIL
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "settings.h"
#include "data.h"
#include "DHSVMerror.h"
#include "functions.h"
#include "massenergy.h"
//#include "snow.h"
#include "constants.h"
//#include "soilmoisture.h"
#include "Calendar.h"

 /*****************************************************************************
   Function name: MassEnergyBalance()

   Purpose      : Calculate mass and energy balance

   Required     :

   Returns      : void

   Modifies     :

   Comments     :

    
 *****************************************************************************/
void CalcMPBuildupWashoff (int y, int x, float DX, float DY,
  int Dt, int InfiltOption, int MaxSoilLayers, int MaxVegLayers, PIXMET *LocalMet,
  ROADSTRUCT *LocalNetwork, PRECIPPIX *LocalPrecip,
  VEGTABLE *VType, VEGPIX *LocalVeg, SOILTABLE *SType,
  SOILPIX *LocalSoil, SNOWPIX *LocalSnow, CHANNEL *ChannelData, MPPIX *LocalMP)
{
  float imperv = VType->ImpervFrac;
  float low = TWP_ON ? TWP_LOW : ATMS_LOW;
  float up = TWP_ON ? TWP_UP : ATMS_UP;
  float source = LocalMP->AtmsMP * DX * DY * Dt / 86400.0 + LocalMP->TWP * TWP_SCALE * Dt;
  /* Piecewise-linear wash-off between runoff thresholds. */
  float ratio = LocalSoil->IExcess <= low ? 0.0 :
    (LocalSoil->IExcess >= up ? 1.0 : (LocalSoil->IExcess - low) / (up - low));

  source += LocalMP->atm_init;
  LocalMP->atm_init = 0.0;
  LocalMP->MPrunoff = source;
  LocalMP->atm_mp += source * (1.0 - imperv);
  LocalMP->Uatm_mp += source * imperv;
  LocalMP->atm_wash = ratio * LocalMP->atm_mp;
  LocalMP->Uatm_wash = ratio * LocalMP->Uatm_mp;
  LocalMP->atm_mp -= LocalMP->atm_wash;
  LocalMP->Uatm_mp -= LocalMP->Uatm_wash;
  LocalMP->mp_accum = LocalMP->atm_mp + LocalMP->Uatm_mp;

}
