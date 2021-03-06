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
    plicFoam

Description
    Solver for 2 incompressible, immiscible fluids using a VOF
    (volume of fluid) phase-fraction based interface capturing with 
    piecewise linear interface reconstruction.        

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "plic.H"
#include "plicFuncs.H"
#include "IOdictionary.H"
#include "centredCPCCellToCellStencilObject.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    #include "setRootCase.H"
    #include "createTime.H"
    #include "createMesh.H"        

    #include "readTimeControls.H"
    //#include "initContinuityErrs.H"
    #include "createFields.H"
    #include "CourantNo.H"
    #include "setInitialDeltaT.H"

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    Foam::plicFuncs::write_field_vector(interface.gradAlpha1());
    Foam::plicFuncs::write_field_vector(interface.nHat());        
    Foam::plicFuncs::write_field_scalar(alpha1);
    Foam::plicFuncs::write_surfaceField_scalar(phiAlpha1,mesh);
    Foam::plicFuncs::write_labelList(interface.cell_phaseState(),mesh,"cell_phaseState");
    Foam::plicFuncs::write_labelList(interface.face_phaseState(),mesh,"face_phaseState");
    Foam::plicFuncs::write_boolList(interface.cell_near_intfc(),mesh,"cell_near_intfc");
    Foam::plicFuncs::write_boolList(interface.face_2ph_flux_needed(),mesh,"face_2ph_flux_needed");

    Info<< "\nStarting time loop\n" << endl;

    while (runTime.run())
    {
        #include "readTimeControls.H"
        #include "CourantNo.H"
        #include "alphaCourantNo.H"
        #include "setDeltaT.H"

        runTime++;

        Info<< "Time = " << runTime.timeName() << nl << endl;
     
        #include "alpha1Eqn.H"
       
        interface.plic_correct();

        runTime.write();

        Foam::plicFuncs::write_field_vector(interface.gradAlpha1());
        Foam::plicFuncs::write_field_vector(interface.nHat());        
        Foam::plicFuncs::write_field_scalar(alpha1);      
        Foam::plicFuncs::write_surfaceField_scalar(phiAlpha1,mesh);
        Foam::plicFuncs::write_labelList(interface.cell_phaseState(),mesh,"cell_phaseState");
        Foam::plicFuncs::write_labelList(interface.face_phaseState(),mesh,"face_phaseState");
        Foam::plicFuncs::write_boolList(interface.cell_near_intfc(),mesh,"cell_near_intfc");
        Foam::plicFuncs::write_boolList(interface.face_2ph_flux_needed(),mesh,"face_2ph_flux_needed");

        Info<< "ExecutionTime = "
            << runTime.elapsedCpuTime()
            << " s\n\n" << endl; 
    }    

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
