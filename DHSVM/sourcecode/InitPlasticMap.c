/*
 * SUMMARY:      InitPlasticMap() - Initialize terrain coverages
 * USAGE:        Part of DHSVM
 *
 * AUTHOR:       Zhuoran Duan 
 * ORG:          PNNL
 * E-MAIL:       
 * ORIG-DATE:    Aug-23
 * DESCRIPTION:  Initialize MP coverages
 * DESCRIP-END.
 * FUNCTIONS:    InitPlasticMap()

 * COMMENTS:
 * $Id: $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "settings.h"
#include "data.h"
#include "DHSVMerror.h"
#include "fileio.h"
#include "functions.h"
#include "constants.h"
#include "getinit.h"
#include "sizeofnt.h"
#include "varid.h"


/*****************************************************************************
  InitPlasticMap()
*****************************************************************************/
static FILE *TWPScaleFile;

void UpdateTWPScale(void)
{
  float value;
  if (!TWPScaleFile)
    return;
  if (fscanf(TWPScaleFile, "%f", &value) != 1 || !isfinite(value) || value < 0.0) {
    fprintf(stderr, "Invalid or exhausted TWP SCALE FILE\n");
    exit(EXIT_FAILURE);
  }
  TWP_SCALE = value;
}

void InitPlasticMap(OPTIONSTRUCT *Options, LISTPTR Input, MAPSIZE *Map, MPPIX ***MPMap)
{
  const char *Routine = "InitPlasticMap";
  char VarName[BUFSIZE + 1];
  int i, x, y, flag, NumberType, reverse, has_atms, has_twp;
  float *values, emission, coefficient, source;
  STRINIENTRY StrEnv[] = {
    {"PLASTIC", "ATMS MAP FILE", "", "none"},
    {"PLASTIC", "TWP MAP FILE", "", "none"},
    {"PLASTIC", "ANTECEDENT DRY DAYS", "", "0"},
    {"PLASTIC", "ATMS MP LOWER THRESH", "", "0"},
    {"PLASTIC", "ATMS MP UPPER THRESH", "", "0.001"},
    {"PLASTIC", "TWP LOWER THRESH", "", "0"},
    {"PLASTIC", "TWP UPPER THRESH", "", "0.001"},
    {"PLASTIC", "TWP SCALE FILE", "", "none"},
    {"PLASTIC", "TWP EMISSION FACTOR MG/VKM", "", "51.1"},
    {"PLASTIC", "TWP SOURCE COEFFICIENT", "", "1"},
    {"PLASTIC", "TWP PARTICLE DIAMETER M", "", "0.000075"},
    {"PLASTIC", "TWP SUBMERGED SPECIFIC DENSITY", "", "0.3"},
    {"PLASTIC", "TWP KINEMATIC VISCOSITY M2/S", "", "0.000001"},
    {"PLASTIC", "TWP VON KARMAN CONSTANT", "", "0.41"},
    {"PLASTIC", "TWP REFERENCE HEIGHT RATIO", "", "0.05"},
    {"PLASTIC", "TWP HORIZONTAL DIFFUSIVITY M2/S", "", "0"},
    {"PLASTIC", "TWP INITIAL BED MASS KG/M2", "", "0"},
    {NULL, NULL, "", NULL}
  };

  if (!(*MPMap = (MPPIX **)calloc(Map->NY, sizeof(MPPIX *))))
    ReportError((char *)Routine, 1);
  for (y = 0; y < Map->NY; y++)
    if (!((*MPMap)[y] = (MPPIX *)calloc(Map->NX, sizeof(MPPIX))))
      ReportError((char *)Routine, 1);
  for (i = 0; StrEnv[i].SectionName; i++) {
    GetInitString(StrEnv[i].SectionName, StrEnv[i].KeyName, StrEnv[i].Default,
      StrEnv[i].VarStr, (unsigned long)BUFSIZE, Input);
    if (IsEmptyStr(StrEnv[i].VarStr))
      ReportError(StrEnv[i].KeyName, 51);
  }

  has_atms = strncmp(StrEnv[atmsmp_file].VarStr, "none", 4);
  has_twp = strncmp(StrEnv[twp_file].VarStr, "none", 4);
  if (!has_atms && !has_twp)
    ReportError("ATMS MAP FILE or TWP MAP FILE", 51);
  if (!(values = (float *)calloc(Map->NX * Map->NY, sizeof(float))))
    ReportError((char *)Routine, 1);

  if (has_atms) {
    GetVarName(910, 0, VarName);
    GetVarNumberType(910, &NumberType);
    flag = Read2DMatrix(StrEnv[atmsmp_file].VarStr, values, NumberType, Map, 0, VarName, 0);
    reverse = Options->FileFormat == NETCDF && flag == 1;
    if (!((Options->FileFormat == NETCDF && (flag == 0 || flag == 1)) || Options->FileFormat == BIN))
      ReportError((char *)Routine, 57);
    for (y = 0; y < Map->NY; y++)
      for (x = 0; x < Map->NX; x++)
        (*MPMap)[y][x].AtmsMP = values[(reverse ? Map->NY - 1 - y : y) * Map->NX + x];
  }

  if (!CopyFloat(&emission, StrEnv[twp_emission].VarStr, 1) || emission <= 0.0)
    ReportError(StrEnv[twp_emission].KeyName, 51);
  if (!CopyFloat(&coefficient, StrEnv[twp_coefficient].VarStr, 1) || coefficient < 0.0)
    ReportError(StrEnv[twp_coefficient].KeyName, 51);
  if (has_twp) {
    memset(values, 0, Map->NX * Map->NY * sizeof(float));
    GetVarName(911, 0, VarName);
    GetVarNumberType(911, &NumberType);
    flag = Read2DMatrix(StrEnv[twp_file].VarStr, values, NumberType, Map, 0, VarName, 0);
    reverse = Options->FileFormat == NETCDF && flag == 1;
    if (!((Options->FileFormat == NETCDF && (flag == 0 || flag == 1)) || Options->FileFormat == BIN))
      ReportError((char *)Routine, 57);
    for (y = 0; y < Map->NY; y++)
      for (x = 0; x < Map->NX; x++) {
        source = values[(reverse ? Map->NY - 1 - y : y) * Map->NX + x];
        /* Paper source equation with m-to-km, mg-to-kg, and day-to-s conversions. */
        (*MPMap)[y][x].TWP = coefficient * emission * fmax(source, 0.0) * Map->DX * 1.e-9 / 86400.0;
      }
  }
  free(values);

  if (!CopyFloat(&DryDays, StrEnv[dry_days].VarStr, 1) || DryDays < 0.0 ||
      !CopyFloat(&ATMS_LOW, StrEnv[atms_low].VarStr, 1) || ATMS_LOW < 0.0 ||
      !CopyFloat(&ATMS_UP, StrEnv[atms_up].VarStr, 1) || ATMS_UP <= ATMS_LOW ||
      !CopyFloat(&TWP_LOW, StrEnv[twp_low].VarStr, 1) || TWP_LOW < 0.0 ||
      !CopyFloat(&TWP_UP, StrEnv[twp_up].VarStr, 1) || TWP_UP <= TWP_LOW ||
      !CopyFloat(&TWP_D50, StrEnv[twp_d50].VarStr, 1) || TWP_D50 <= 0.0 ||
      !CopyFloat(&TWP_DENSITY, StrEnv[twp_density].VarStr, 1) || TWP_DENSITY <= 0.0 ||
      !CopyFloat(&TWP_VISCOSITY, StrEnv[twp_viscosity].VarStr, 1) || TWP_VISCOSITY <= 0.0 ||
      !CopyFloat(&TWP_KAPPA, StrEnv[twp_kappa].VarStr, 1) || TWP_KAPPA <= 0.0 ||
      !CopyFloat(&TWP_REF_RATIO, StrEnv[twp_ref_ratio].VarStr, 1) || TWP_REF_RATIO <= 0.0 || TWP_REF_RATIO >= 1.0 ||
      !CopyFloat(&TWP_DIFFUSIVITY, StrEnv[twp_diffusivity].VarStr, 1) || TWP_DIFFUSIVITY < 0.0 ||
      !CopyFloat(&TWP_BED_INIT, StrEnv[twp_bed_init].VarStr, 1) || TWP_BED_INIT < 0.0)
    ReportError("PLASTIC", 51);

  TWP_ON = has_twp;
  TWP_SCALE = 1.0;
  if (strncmp(StrEnv[twp_scale_file].VarStr, "none", 4)) {
    TWPScaleFile = fopen(StrEnv[twp_scale_file].VarStr, "r");
    if (!TWPScaleFile)
      ReportError(StrEnv[twp_scale_file].VarStr, 5);
  }
  for (y = 0; y < Map->NY; y++)
    for (x = 0; x < Map->NX; x++)
      (*MPMap)[y][x].atm_init = DryDays * 86400.0 *
        ((*MPMap)[y][x].AtmsMP * Map->DX * Map->DY / 86400.0 + (*MPMap)[y][x].TWP);
}


