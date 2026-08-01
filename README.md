# DHSVM-MP-TWP

This repository is a research fork of the Distributed Hydrology Soil
Vegetation Model (DHSVM) with a microplastics module focused on
tire wear particles (TWP). The hydrologic model remains DHSVM; setting
`PLASTICS = FALSE` disables the added particle calculations.

## Publications

Li, Z., Huang, Y., Sun, N., Duan, Z., Wigmosta, M., Yang, Z., & Maurer, B. (2026). Traffic-informed prioritization of tire and road wear particle monitoring and mitigation in the Delaware River Basin and Philadelphia metropolitan region. Submitted to Journal of Environmental Management.


## TWP additions

The TWP module adds:

- an optional airborne-microplastic deposition pathway;
- a gridded, cell-aggregated annual average daily traffic (AADT) source map;
- antecedent dry-period accumulation and runoff-dependent wash-off;
- division of source mass between impervious and pervious surface stores;
- conservative transfer of mobilized particle mass to the stream network;
- advective channel routing with optional link-to-link diffusion;
- water-column settling, bed deposition, and bed resuspension;
- time-varying TWP source scaling; and
- TWP concentration, load, spatial-state, and mass-balance outputs.


## Repository scope

This repository intentionally contains only the core model source. It excludes the test datasets,
RBM model, auxiliary conversion programs, legacy GIS scripts, historical
tutorials, and generated build products. 

## Activating the particle module

Set the following in the DHSVM input file:

```ini
[OPTIONS]
PLASTICS = TRUE
```

Add a `[PLASTIC]` section. `TWP MAP FILE` activates the TWP pathway. `ATMS MAP
FILE` controls the older airborne-microplastics pathway and can be `none`.

```ini
[PLASTIC]
ATMS MAP FILE = none
TWP MAP FILE = /absolute/path/to/aggregated_aadt.bin
ANTECEDENT DRY DAYS = 7
ATMS MP LOWER THRESH = 0
ATMS MP UPPER THRESH = 0.001
TWP LOWER THRESH = 0.0001
TWP UPPER THRESH = 0.001
TWP SCALE FILE = none
TWP EMISSION FACTOR MG/VKM = 51.1
TWP SOURCE COEFFICIENT = 1
TWP PARTICLE DIAMETER M = 0.000075
TWP SUBMERGED SPECIFIC DENSITY = 0.3
TWP KINEMATIC VISCOSITY M2/S = 0.000001
TWP VON KARMAN CONSTANT = 0.41
TWP REFERENCE HEIGHT RATIO = 0.05
TWP HORIZONTAL DIFFUSIVITY M2/S = 0.1
TWP INITIAL BED MASS KG/M2 = 0
```

## Configuration reference

| Key | Unit or format | Default | Purpose |
|---|---|---:|---|
| `ATMS MAP FILE` | DHSVM raster | `none` | Airborne MP deposition rate in kg m^-2 day^-1. |
| `TWP MAP FILE` | DHSVM raster | `none` | Cell-aggregated AADT in vehicles day^-1. For NetCDF, use variable `MP.TWPmp`. |
| `ANTECEDENT DRY DAYS` | day | `0` | Initial dry-period source accumulation. |
| `ATMS MP LOWER THRESH` | m timestep^-1 | `0` | Runoff depth at or below which airborne MP wash-off is zero. |
| `ATMS MP UPPER THRESH` | m timestep^-1 | `0.001` | Runoff depth at or above which airborne MP wash-off is complete. |
| `TWP LOWER THRESH` | m timestep^-1 | `0` | Runoff depth at or below which TWP wash-off is zero. |
| `TWP UPPER THRESH` | m timestep^-1 | `0.001` | Runoff depth at or above which TWP wash-off is complete. Must exceed the lower threshold. |
| `TWP SCALE FILE` | text file or `none` | `none` | One nonnegative source multiplier per timestep. The run stops for an invalid or exhausted file. |
| `TWP EMISSION FACTOR MG/VKM` | mg vehicle^-1 km^-1 | `51.1` | tire wear emission factor. |
| `TWP SOURCE COEFFICIENT` | dimensionless | `1` | Source adjustment coefficient (`f_ct`). |
| `TWP PARTICLE DIAMETER M` | m | `0.000075` | Median particle diameter (`d50`). |
| `TWP SUBMERGED SPECIFIC DENSITY` | dimensionless | `0.3` | `(rho_p-rho_w)/rho_w`; 0.3 corresponds to 1300 kg m^-3 particles in 1000 kg m^-3 water. |
| `TWP KINEMATIC VISCOSITY M2/S` | m^2 s^-1 | `0.000001` | Water kinematic viscosity. |
| `TWP VON KARMAN CONSTANT` | dimensionless | `0.41` | Von Karman constant used in the Rouse number. |
| `TWP REFERENCE HEIGHT RATIO` | dimensionless | `0.05` | Nominal reference height divided by flow depth. |
| `TWP HORIZONTAL DIFFUSIVITY M2/S` | m^2 s^-1 | `0` | Stream-link longitudinal diffusivity; zero disables diffusion. |
| `TWP INITIAL BED MASS KG/M2` | kg m^-2 | `0` | Initial re-entrainable TWP bed mass for every stream segment. |

## Preparing the TWP map

The TWP raster represents cell-aggregated AADT, not a point-station table:

1. Project traffic-count locations into the DHSVM grid coordinate system.
2. Assign every station to a DHSVM cell.
3. Sum AADT where multiple stations fall in the same cell.
4. Write a raster with the dimensions, orientation, and format of the other
   DHSVM input maps.
5. Use zero for cells without a source; do not use missing values inside the
   modeled basin.
6. For temporal variation, provide one nonnegative scale value for every model
   timestep.

The source conversion is:

```text
P_TWP = f_ct E AADT DX / 1000
q_TWP = P_TWP 1e-6 / 86400
```

Here `P_TWP` is mg day^-1, `q_TWP` is kg s^-1, `E` is the emission factor in
mg vehicle^-1 km^-1, and `DX` is grid spacing in meters. The temporal scale
factor is applied to `q_TWP` each timestep.

Wash-off is linear between the configured runoff thresholds:

```text
F = 0                                      IExcess <= lower
F = (IExcess-lower)/(upper-lower)          lower < IExcess < upper
F = 1                                      IExcess >= upper
```

## Channel transport

Hydrologic network routing supplies advection. Optional link-to-link diffusion
uses:

```text
M_diff = epsilon_s A (C_up-C_down) Dt / L
```

The stream segment is treated as rectangular. Flow depth is the greater of
storage depth and the Manning normal depth calculated from routed flow, width,
slope, and roughness. Particle settling velocity is evaluated from particle
diameter, submerged density, and water viscosity. The water-column/near-bed
concentration ratio uses a numerically integrated Rouse profile.

The equilibrium reference concentration follows the van Rijn suspended-load
formulation. Water-column/bed exchange is:

```text
Delta M = ws (Ceq-Ca) width length Dt
```

Positive exchange entrains stored bed mass; negative exchange deposits
suspended mass. Entrainment is limited by available bed mass and deposition by
available suspended mass, preserving mass. The formulation follows L. C. van
Rijn (1984), *Journal of Hydraulic Engineering* 110(11), 1613-1641,
DOI [10.1061/(ASCE)0733-9429(1984)110:11(1613)](https://doi.org/10.1061/(ASCE)0733-9429(1984)110:11(1613)).

## TWP outputs

| File or variable | Quantity | Unit |
|---|---|---|
| `TWP.Load.Only` | TWP mass leaving each recorded stream segment per timestep | kg |
| `TWP.Conc.Only` | Mixed depth-averaged TWP concentration | kg m^-3 |
| `MP.TWPmp` | Input TWP/AADT raster variable | vehicles day^-1 per cell |
| `MP.mp_accum` | Accumulated surface microplastic state | model state unit |

For concentration conversion, `1 kg m^-3 = 1000 mg L^-1`.

The DHSVM mass-balance output also includes particle input, surface storage,
channel export, and residual/error terms. Review the residual when validating a
new setup.

## Constraints and current limitations

- `TWP UPPER THRESH` must be greater than `TWP LOWER THRESH`.
- Particle diameter, submerged density, viscosity, and the von Karman constant
  must be positive.
- The reference-height ratio must be between zero and one.
- Diffusivity and initial bed mass cannot be negative.
- TWP map values and temporal scale factors cannot be negative.
- A complete TWP simulation requires a routed stream network. Unit-hydrograph
  mode does not perform channel particle routing.
- The TWP implementation is a research extension and should be checked against
  field data and independent mass-balance calculations before operational use.

## License and attribution

Retain the upstream DHSVM attribution and license terms that apply to this
source. Publications using this fork should cite DHSVM, the relevant TWP or
microplastics methodology, and the exact Git commit or release tag used.
