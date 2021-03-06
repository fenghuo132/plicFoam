/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2013 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Application
    calcRho

Description
    Generate lookup table for density

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "PR_EoS.h"
#include "MACROS2.H"
#include "plic.H"
#include "plicFuncs.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    #include "setRootCase.H"
    #include "createTime.H"
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    double *Pc, *Tc, *Vc, *w, *MW, *Tb, *SG, *H8, *k, *dm;
    double Ta_kij, Tn_kij, P, Ta, Tn, T, dT, Ya, Yn, Y, dY, rho_tmp;
    int n, nT_kij, nT, nY, i, j, iT, iY, idx;
    double *kij, *kij_T;
    double *y, *x;

    int n_species_db, iread;
    char str_tmp[50];
    FILE *f;
    int n_pseudospecies_db, n_purespecies_db;
    int *idx_species, *idx_groups;

    f = fopen("constant/BIP_nT.dat+", "r");
    iread = fscanf(f, "%lf %lf %d", &Ta_kij, &Tn_kij, &nT_kij);
    fclose(f);

    Info<< "Ta_kij = " << Ta_kij << " Tn_kij = " << Tn_kij << " nT_kij = " << nT_kij << endl;

    f = fopen("constant/calcRho_input.dat+", "r");
    iread = fscanf(f, "%d %lf %lf %lf %d %lf %lf %d", &n, &P, &Ta, &Tn, &nT, &Ya, &Yn, &nY);    
    fclose(f);

    Info<< "number of species = " << n << endl;

    //Allocate memory for arrays
    _NNEW2_(kij, double, n*n);
    _NNEW2_(kij_T, double, nT_kij*n*n);
    _NNEW2_(Pc, double, n);
    _NNEW2_(Tc, double, n);
    _NNEW2_(Vc, double, n);
    _NNEW2_(w, double, n);
    _NNEW2_(MW, double, n);
    _NNEW2_(Tb, double, n);
    _NNEW2_(SG, double, n);
    _NNEW2_(H8, double, n);
    _NNEW2_(k, double, n);
    _NNEW2_(dm, double, n);
    _NNEW2_(idx_species, int, n);
    _NNEW2_(idx_groups, int, n);
    _NNEW2_(y, double, n);
    _NNEW2_(x, double, n);

    //Read thermo input parameters from files       
    f = fopen("constant/species.dat+", "r");

    iread = fscanf(f, "%d %d", &n_pseudospecies_db, &n_purespecies_db);
    if(iread <= 0) Info<< "Input file reading error-----------------" << endl;
    n_species_db = n_pseudospecies_db + n_purespecies_db;

    Info<< "n_pseudoSpecies_db = " << n_pseudospecies_db << "  n_purespecies_db = " << n_purespecies_db << endl;

    fclose(f);

    f = fopen("constant/input.dat+", "r");

    iread=fscanf(f, "%s %d", str_tmp, &n);  // num of species
    if(iread <= 0) Info<< "Input file reading error-----------------" << endl;

    iread=fscanf(f, "%s", str_tmp);         // index of species
    for (i=0; i<n; i++) iread=fscanf(f, "%d", &idx_species[i]);
    if(iread <= 0) Info<< "Input file reading error-----------------" << endl;

    iread=fscanf(f, "%s", str_tmp);         // index of PPR78 groups
    for (i=0; i<n; i++) iread=fscanf(f, "%d", &idx_groups[i]);
    if(iread <= 0) Info<< "Input file reading error-----------------" << endl;

    Info<< "Species indices in database:  ";
    for(i=0; i<n; i++) Info<< idx_species[i] << "  ";
    Info<< endl;

    Info<< "Species group indices in database:  ";
    for(i=0; i<n; i++) Info<< idx_groups[i] << "  ";
    Info<< endl;

    fclose(f);

    f = fopen("constant/petro.dat+","r");

    for (j=0;j<n;j++) 
    {
        for (i=0;i<n_species_db;i++) 
        {
            double tmp;
            double xPNA[3];

            //[x' y' Tb' Tc' Pc' w' MW' Vc' SG' H_8' xP' xN' xA'];
            iread=fscanf(f, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
            &tmp, &tmp, &Tb[j], &Tc[j], &Pc[j], &w[j], &MW[j], &Vc[j], 
            &SG[j], &H8[j], &xPNA[0], &xPNA[1], &xPNA[2]);

            if(iread <= 0) Info<< "Input file reading error-----------------" << endl;

            //tmp = 5.811+4.919*w[j];
            //Vc[j] = 83.14*Tc[j]/Pc[j]/(3.72+.26*(tmp-7)); // cm^3/mol

            Pc[j] *= 1e5;  // bar --> Pa
            
            dm[j] = 0.0; 
            k [j] = 0.0;

            if (i==idx_species[j]) 
            {
                // SPECIAL DATA POINTS BUILT IN THE CODE
                // 1. water
                if (fabs(Tc[j]-647)<0.5 && fabs(Pc[j]-220.6e5)<0.5e5) 
                {
                    Vc[j] = 57.1; // cm^3/mol
                    dm[j] = 1.85; // Debye
                    k [j] = 0.076;

                }                                
                // 2. toluene
                else if (fabs(Tc[j]-591.8)<0.5 && fabs(Pc[j]-41.1e5)<0.5e5)
                {
                    Vc[j] = 316.0;
                    dm[j] = 0.36; // Debye

                }// 3. n-decane
                else if (fabs(Tc[j]-617.7)<0.5 && fabs(Pc[j]-21.1e5)<0.5e5)
                {
                    Vc[j] = 600.0;
                }// 4. n-C30
                else if (fabs(Tc[j]-844.0)<0.5 && fabs(Pc[j]- 8.0e5)<0.5e5) {
                    Vc[j] = 1805.0;

                    // 5. n-C50
                }else if (fabs(Tc[j]-1073.6)<0.5&& fabs(Pc[j]-3.51e5)<0.5e5) {
                    Vc[j] = 2999;

                    // 6. Benzene-C10
                }else if (fabs(Tc[j]-753.0)<0.5 && fabs(Pc[j]-17.7e5)<0.5e5) {
                    Vc[j] = 813;

                    // 7. Benzene-C30
                }else if (fabs(Tc[j]-965.6)<0.5 && fabs(Pc[j]- 5.3e5)<0.5e5) {
                    Vc[j] = 1943.5;

                    // 8. Naphthalene-C10
                }else if (fabs(Tc[j]-859.0)<0.5 && fabs(Pc[j]-15.8e5)<0.5e5) {
                    Vc[j] = 1070;

                    // 9. Naphthalene-C12
                }else if (fabs(Tc[j]-854.6)<0.5 && fabs(Pc[j]-13.0e5)<0.5e5) {
                    Vc[j] = 1081.5;

                    // 10. Benzene
                }else if (fabs(Tc[j]-562.0)<0.5 && fabs(Pc[j]-49.0e5)<0.5e5) {
                    Vc[j] = 256.0;

                    // 11. o-xylene
                }else if (fabs(Tc[j]-630.3)<0.5 && fabs(Pc[j]-37.3e5)<0.5e5) {
                    Vc[j] = 370.0;

                    // 12. p-xylene
                }else if (fabs(Tc[j]-616.2)<0.5 && fabs(Pc[j]-35.1e5)<0.5e5) {
                    Vc[j] = 378.0;

                    // 13. 1,3,5-trimethylbenzene
                }else if (fabs(Tc[j]-637.3)<0.5 && fabs(Pc[j]-31.3e5)<0.5e5) {
                    Vc[j] = 433.0;

                    // 14. naphthalene
                }else if (fabs(Tc[j]-748.4)<0.5 && fabs(Pc[j]-40.5e5)<0.5e5) {
                    Vc[j] = 407.0;

                    // 15. 1-methylnaphthalene
                }else if (fabs(Tc[j]-772.0)<0.5 && fabs(Pc[j]-36.0e5)<0.5e5) {
                    Vc[j] = 465;

                    // 16. anthracene
                }else if (fabs(Tc[j]-873.0)<0.5 && fabs(Pc[j]-29.0e5)<0.5e5) {
                    Vc[j] = 554;

                    // 17. 1,2-diphenylethane
                }else if (fabs(Tc[j]-780.0)<0.5 && fabs(Pc[j]-26.5e5)<0.5e5) {
                    Vc[j] = 616;

                    // 18. pyrene
                }else if (fabs(Tc[j]-936.0)<0.5 && fabs(Pc[j]-26.1e5)<0.5e5) {
                    Vc[j] = 660;

                    // 19. n-C16
                }else if (fabs(Tc[j]-723.0)<0.5 && fabs(Pc[j]-14.0e5)<0.5e5) {
                    Vc[j] = 969.2;

                    // 20. trans-decalin
                }else if (fabs(Tc[j]-687.0)<0.5 && fabs(Pc[j]-32.0e5)<0.5e5) {
                    Vc[j] = 480.0;

                    // 21. butylbenzene
                }else if (fabs(Tc[j]-660.5)<0.5 && fabs(Pc[j]-28.9e5)<0.5e5) {
                    Vc[j] = 497.0;

                    // 22. hexylbenzene
                }else if (fabs(Tc[j]-698.0)<0.5 && fabs(Pc[j]-23.8e5)<0.5e5) {
                    Vc[j] = 593.0;
                }
                // WHEN EQUALS, WE FOUND THE DATA 
                // SO QUIT THE READING FOR SPECIES I
                break;
            }
        }
        rewind(f);
    }

    fclose(f);

    f = fopen("constant/kij_T.dat+", "r");

    for(iT=0; iT<nT_kij; iT++)
    {
        for(i=0; i<n; i++)
        {
            for(j=0; j<n; j++)
            {
                idx = iT*n*n + i*n + j;
                iread = fscanf(f, "%lf", &kij_T[idx]);
            }
        }
    }
    
    fclose(f);
    
    f = fopen("rho_Y_T.dat+", "w");

    dY = (Yn - Ya)/(nY - 1);
    dT = (Tn - Ta)/(nT - 1);

    for(iT=0; iT<nT; iT++)
    {
        T = Ta + iT*dT;

        Info<< "T = " << T << endl;
        Info<< "kij: ";

        plicFuncs::calc_kij_from_table(T, n, Ta, Tn, nT_kij, kij_T, kij);

        for(i=0; i<n; i++)
        {
            for(j=0; j<n; j++)
            {
                idx = i*n + j;
                Info<< kij[idx] << " ";
            }
        }
        Info<< endl;

        for(iY=0; iY<nY; iY++)
        {
            Y = Ya + iY*dY;
            y[0] = Y; y[1] = 1 - Y;
            Info<< "y: ";
            for(i=0; i<n; i++)
            {
                Info<< y[i] << " ";
            }

            plicFuncs::y2x(n, MW, y, x);
            density_pr_eos2_(&P, &T, &n, Pc, Tc, w, kij, MW, x, &rho_tmp);

            Info<< " rho = " << rho_tmp << endl;
            fprintf(f, "%lf ", rho_tmp);
        }

        fprintf(f, "\n");
    }

    fclose(f);

    _DDELETE2_(idx_species);
    _DDELETE2_(idx_groups);

    _DDELETE2_(kij);
    _DDELETE2_(kij_T);
    _DDELETE2_(Pc);
    _DDELETE2_(Tc);
    _DDELETE2_(Vc);
    _DDELETE2_(w);
    _DDELETE2_(MW);
    _DDELETE2_(dm);
    _DDELETE2_(k);
    _DDELETE2_(H8);
    _DDELETE2_(Tb);
    _DDELETE2_(SG);

    _DDELETE2_(y);
    _DDELETE2_(x);
   
    Info<< "ExecutionTime = "
        << runTime.elapsedCpuTime()
        << " s\n\n" << endl;     

    Info<< "End\n" << endl;    

    return 0;
}


// ************************************************************************* //
