// Copyright 2021-2022 Lawrence Livermore National Security, LLC and other ExaCA Project Developers.
// See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#include "GAprint.hpp"
#include "GAutils.hpp"

//*****************************************************************************/
void PrintMisorientationData(bool *AnalysisTypes, std::string BaseFileName, int XMin, int XMax, int YMin, int YMax,
                             int ZMin, int ZMax, ViewI3D_H LayerID, ViewF_H GrainUnitVector, ViewI3D_H GrainID,
                             int NumberOfOrientations) {

    // Frequency of misorientations in the selected region
    std::ofstream MisorientationPlot;
    std::string FNameM = BaseFileName + "_MisorientationFrequency.csv";
    if (AnalysisTypes[0]) {
        MisorientationPlot.open(FNameM);
        std::cout << "Printing file " << FNameM
                  << " of grain misorientations relative to the +Z direction, for all cells in selected volume (not "
                     "including substrate)"
                  << std::endl;
    }
    int NumberOfMeltedCells = 0;
    long double MisorientationSum = 0.0;
    int NumberOfMeltedCellsTop = 0;
    long double MisorientationSumTop = 0.0;
    ViewF_H GrainMisorientation = MisorientationCalc(NumberOfOrientations, GrainUnitVector, 2);
    for (int k = ZMin; k <= ZMax; k++) {
        for (int i = XMin; i <= XMax; i++) {
            for (int j = YMin; j <= YMax; j++) {
                // Only take data from cells in the representative area that underwent melting (LayerID >= 0)
                if (LayerID(k, i, j) != -1) {
                    int MyOrientation = getGrainOrientation(GrainID(k, i, j), NumberOfOrientations);
                    float MyMisorientation = GrainMisorientation(MyOrientation);
                    if (AnalysisTypes[0])
                        MisorientationPlot << MyMisorientation << std::endl;
                    MisorientationSum += MyMisorientation;
                    if (k == ZMax) {
                        MisorientationSumTop += MyMisorientation;
                        NumberOfMeltedCellsTop++;
                    }
                    NumberOfMeltedCells++;
                }
            }
        }
    }
    if (AnalysisTypes[0])
        MisorientationPlot.close();
    std::cout << "Within the representative region consisting of "
              << (XMax - XMin + 1) * (YMax - YMin + 1) * (ZMax - ZMin + 1) << " cells, " << NumberOfMeltedCells
              << " underwent melting and:" << std::endl;
    std::cout << "-- The mean misorientation relative to the +Z direction is "
              << MisorientationSum / ((double)(NumberOfMeltedCells)) << " degrees" << std::endl;
    std::cout << "-- The mean misorientation relative to the +Z direction at the representative region top (Z = "
              << ZMax << ") is " << MisorientationSumTop / ((double)(NumberOfMeltedCellsTop)) << " degrees"
              << std::endl;
}

//*****************************************************************************/

void PrintSizeData(bool *AnalysisTypes, std::string BaseFileName, int XMin, int XMax, int YMin, int YMax, int ZMin,
                   int ZMax, int nx, int ny, int nz, ViewI3D_H, ViewI3D_H GrainID, double deltax) {

    // Get vector of unique GrainIDs
    int Counter = 0;
    int NucleatedGrainCells = 0;
    int DomainVol = (ZMax - ZMin + 1) * (YMax - YMin + 1) * (XMax - XMin + 1);
    std::vector<int> AllGrainIDs(DomainVol);
    for (int k = ZMin; k <= ZMax; k++) {
        for (int i = XMin; i <= XMax; i++) {
            for (int j = YMin; j <= YMax; j++) {
                AllGrainIDs[Counter] = GrainID(k, i, j);
                Counter++;
                if (GrainID(k, i, j) < 0)
                    NucleatedGrainCells++;
            }
        }
    }
    std::vector<int> UniqueGrainIDs = FindUniqueGrains(AllGrainIDs);
    int NumberOfGrains = UniqueGrainIDs.size();
    double AvgVolPerGrain = DivideCast<double>(DomainVol, NumberOfGrains);
    double AvgVolPerGrain_Norm = AvgVolPerGrain * deltax * deltax * pow(10, 12);
    std::cout << "-- There are " << NumberOfGrains << " grains in this volume, and the mean grain volume is "
              << AvgVolPerGrain_Norm << " cubic microns" << std::endl;
    double VolFractNucGrains = DivideCast<double>(NucleatedGrainCells, DomainVol);
    std::cout << "-- The volume fraction consisting of nucleated grains is " << VolFractNucGrains << std::endl;

    float *AspectRatio = new float[NumberOfGrains];
    int *VolGrain = new int[NumberOfGrains];
    float *GrainHeight = new float[NumberOfGrains];
    float ARSum = 0.0;
    float VolWtARSum = 0.0;
    float GrainHeightSum = 0.0;
    float GrainWidthSum = 0.0;
    for (int n = 0; n < NumberOfGrains; n++) {
        AspectRatio[n] = 0;
        VolGrain[n] = 0;
        int TempTopX = 0;
        int TempTopY = 0;
        int TempTopZ = 0;
        int TempBottomX = nx - 1;
        int TempBottomY = ny - 1;
        int TempBottomZ = nz - 1;
        int ThisGrainID = UniqueGrainIDs[n];
        for (int k = ZMin; k <= ZMax; k++) {
            for (int i = XMin; i <= XMax; i++) {
                for (int j = YMin; j <= YMax; j++) {
                    // Only take data from cells in the representative area that underwent melting
                    if (GrainID(k, i, j) == ThisGrainID) {
                        VolGrain[n]++;
                        if (i > TempTopX)
                            TempTopX = i;
                        if (i < TempBottomX)
                            TempBottomX = i;
                        if (j > TempTopY)
                            TempTopY = j;
                        if (j < TempBottomY)
                            TempBottomY = j;
                        if (k > TempTopZ)
                            TempTopZ = k;
                        if (k < TempBottomZ)
                            TempBottomZ = k;
                    }
                }
            }
        }
        GrainHeight[n] = TempTopZ - TempBottomZ + 1;
        float GrainWidthX = TempTopX - TempBottomX + 1;
        float GrainWidthY = TempTopY - TempBottomY + 1;
        GrainHeightSum += GrainHeight[n];
        GrainWidthSum += 0.5 * (GrainWidthX + GrainWidthY);
        float AR_XZ = (float)(TempTopZ - TempBottomZ + 1) / (float)(TempTopX - TempBottomX + 1);
        float AR_YZ = (float)(TempTopZ - TempBottomZ + 1) / (float)(TempTopY - TempBottomY + 1);
        AspectRatio[n] = 0.5 * (AR_XZ + AR_YZ);
        ARSum += AspectRatio[n];
        VolWtARSum += AspectRatio[n] * (float)(VolGrain[n]);
    }
    std::cout << "-- The mean grain aspect ratio (Z direction to transverse) is " << ARSum / (float)(NumberOfGrains)
              << std::endl;
    std::cout << "-- The mean volume-weighted grain aspect ratio (Z direction to transverse) is "
              << VolWtARSum / ((float)(DomainVol)) << std::endl;
    std::cout << "-- The mean grain height is " << GrainHeightSum / (float)(NumberOfGrains) << " microns" << std::endl;
    std::cout << "-- The mean grain width is " << GrainWidthSum / (float)(NumberOfGrains) << " microns" << std::endl;

    if (AnalysisTypes[1]) {
        std::ofstream VolumePlot;
        std::string FNameV = BaseFileName + "_VolumeFrequency.csv";
        VolumePlot.open(FNameV);
        std::cout << "Printing file " << FNameV << " of grain volumes (in cubic microns) in selected volume"
                  << std::endl;
        for (int n = 0; n < NumberOfGrains; n++) {
            VolumePlot << VolGrain[n] * deltax * deltax * pow(10, 12) << std::endl;
        }
        VolumePlot.close();
    }
    if (AnalysisTypes[2]) {
        std::ofstream ARPlot;
        std::string FNameAR = BaseFileName + "_AspectRatioFrequency.csv";
        ARPlot.open(FNameAR);
        std::cout << "Printing file " << FNameAR << " of grain aspect ratios in selected volume" << std::endl;
        for (int n = 0; n < NumberOfGrains; n++) {
            ARPlot << AspectRatio[n] << std::endl;
        }
        ARPlot.close();
    }
    if (AnalysisTypes[5]) {
        std::ofstream GrainHeightPlot;
        std::string FNameGH = BaseFileName + "_GrainHeightDistribution.csv";
        std::cout << "Printing file " << FNameGH << " of mean height distribution (in microns) for the selected volume"
                  << std::endl;
        GrainHeightPlot.open(FNameGH);
        for (int n = 0; n < NumberOfGrains; n++) {
            GrainHeightPlot << GrainHeight[n] << std::endl;
        }
        GrainHeightPlot.close();
    }
}

//*****************************************************************************/
void PrintGrainAreaData(bool *AnalysisTypes, std::string BaseFileName, double deltax, int XMin, int XMax, int YMin,
                        int YMax, int ZMin, int ZMax, ViewI3D_H GrainID) {

    std::string FName1 = BaseFileName + "_GrainAreas.csv";
    std::string FName2 = BaseFileName + "_WeightedGrainAreas.csv";
    std::ofstream Grainplot1, Grainplot2;
    if (AnalysisTypes[3]) {
        std::cout << "Printing file " << FName1 << " of grain area values (in square microns) for all Z coordinates"
                  << std::endl;
        Grainplot1.open(FName1);
    }
    if (AnalysisTypes[4]) {
        std::cout << "Printing file " << FName2
                  << " of weighted grain area values (in square microns) for every 5th Z coordinate" << std::endl;
        Grainplot2.open(FName2);
    }

    if (!(AnalysisTypes[2]) && (!(AnalysisTypes[3])))
        ZMin = ZMax; // only print grain area/weighted grain area to screen at this one Z coordinate

    int LayerArea = (XMax - XMin + 1) * (YMax - YMin + 1);
    for (int k = ZMin; k <= ZMax; k++) {
        std::vector<int> GIDAllVals_ThisLayer(LayerArea);
        int Counter = 0;
        for (int i = XMin; i <= XMax; i++) {
            for (int j = YMin; j <= YMax; j++) {
                GIDAllVals_ThisLayer[Counter] = GrainID(k, i, j);
                Counter++;
            }
        }
        // List of unique grain ID values from the list of all values
        std::vector<int> GIDVals_ThisLayer = FindUniqueGrains(GIDAllVals_ThisLayer);
        int GrainsThisLayer = GIDVals_ThisLayer.size();
        double MeanGrainAreaThisLayer = DivideCast<double>(LayerArea, GrainsThisLayer);
        if (k == ZMax) {
            std::cout << "Number of grains at the representative region top (Z = " << ZMax << "): " << GrainsThisLayer
                      << std::endl;
            if (AnalysisTypes[6]) {
                std::string FName3 = BaseFileName + "_GrainWidthDistributionX.csv";
                std::string FName4 = BaseFileName + "_GrainWidthDistributionY.csv";
                std::ofstream Grainplot3;
                std::ofstream Grainplot4;
                std::cout << "Printing files " << FName3 << " and " << FName4
                          << " of grain width distributions in x and y (in microns) at Z = " << ZMax << std::endl;
                Grainplot3.open(FName3);
                Grainplot4.open(FName4);
                for (int n = 0; n < GrainsThisLayer; n++) {
                    int ThisGrainID = GIDVals_ThisLayer[n];
                    int TempTopX = XMin;
                    int TempTopY = YMin;
                    int TempBottomX = XMax;
                    int TempBottomY = YMax;
                    for (int i = XMin; i <= XMax; i++) {
                        for (int j = YMin; j <= YMax; j++) {
                            if (GrainID(k, i, j) == ThisGrainID) {
                                if (i > TempTopX)
                                    TempTopX = i;
                                if (i < TempBottomX)
                                    TempBottomX = i;
                                if (j > TempTopY)
                                    TempTopY = j;
                                if (j < TempBottomY)
                                    TempBottomY = j;
                            }
                        }
                    }
                    float GrainExtentX = TempTopX - TempBottomX + 1;
                    float GrainExtentY = TempTopY - TempBottomY + 1;
                    Grainplot3 << GrainExtentX * deltax / pow(10, -6) << std::endl;
                    Grainplot4 << GrainExtentY * deltax / pow(10, -6) << std::endl;
                }
                Grainplot3.close();
                Grainplot4.close();
            }
        }
        if (((AnalysisTypes[3]) && (k % 5 == 0)) || (k == ZMax)) {
            long int AreaXArea = 0;
            for (int l = 0; l < GrainsThisLayer; l++) {
                long int MyGrainArea = 0;
                for (int ll = 0; ll < LayerArea; ll++) {
                    if (GIDVals_ThisLayer[l] == GIDAllVals_ThisLayer[ll])
                        MyGrainArea++;
                }
                AreaXArea += MyGrainArea * MyGrainArea;
            }
            double WeightedArea = DivideCast<double>(AreaXArea, LayerArea);
            if (AnalysisTypes[3])
                Grainplot2 << WeightedArea * deltax * deltax / pow(10, -12) << std::endl;
            if (k == ZMax)
                std::cout << "-- The mean weighted grain area at the representative region top (Z = " << ZMax << ") is "
                          << WeightedArea * deltax * deltax / pow(10, -12) << " square microns" << std::endl;
        }
        if (AnalysisTypes[4])
            Grainplot1 << MeanGrainAreaThisLayer * deltax * deltax / pow(10, -12) << std::endl;
        if (k == ZMax)
            std::cout << "-- The mean grain area at the representative region top (Z = " << ZMax << ") is "
                      << MeanGrainAreaThisLayer * deltax * deltax / pow(10, -12) << " square microns" << std::endl;
    }
    if (AnalysisTypes[3])
        Grainplot1.close();
    if (AnalysisTypes[4])
        Grainplot2.close();
}

//*****************************************************************************/
void PrintPoleFigureData(bool *AnalysisTypes, std::string BaseFileName, int NumberOfOrientations, int XMin, int XMax,
                         int YMin, int YMax, int ZMin, int ZMax, ViewI3D_H GrainID, ViewI3D_H LayerID,
                         bool NewOrientationFormatYN, ViewF_H GrainEulerAngles) {

    if (AnalysisTypes[7]) {

        // Histogram of orientations for texture determination
        ViewI_H GOHistogram("GOHistogram", NumberOfOrientations);
        // frequency data on grain ids
        for (int k = ZMin; k <= ZMax; k++) {
            for (int j = YMin; j <= YMax; j++) {
                for (int i = XMin; i <= XMax; i++) {
                    if (LayerID(k, i, j) != -1) {
                        int GOVal = (abs(GrainID(k, i, j)) - 1) % NumberOfOrientations;
                        GOHistogram(GOVal)++;
                    }
                }
            }
        }
        if (NewOrientationFormatYN) {
            // Write pole figure data for this region using the new format
            std::string FNameM = BaseFileName + "_PFVolumeX" + std::to_string(XMin) + "-" + std::to_string(XMax) + "Y" +
                                 std::to_string(YMin) + "-" + std::to_string(YMax) + "Z" + std::to_string(ZMin) + "-" +
                                 std::to_string(ZMax) + ".txt";
            WritePoleFigureDataToFile(FNameM, NumberOfOrientations, GrainEulerAngles, GOHistogram);
        }
        else {
            // Write pole figure data for this region using the old format
            std::string FNameM = BaseFileName + "_MTEXOrientations.csv";
            WritePoleFigureDataToFile_OldFormat(FNameM, NumberOfOrientations, GOHistogram);
        }
    }
}

// Helper function for unimodal analysis of the grains in the specified cross-section
void AnalyzeCrossSection_Unimodal(std::ofstream &QoIs, std::string BaseFileName, std::string ThisCrossSectionPlane,
                                  double deltax, int NumberOfGrains, int CrossSectionSize, std::vector<int> GrainAreas,
                                  float MinGrainSize_microns = 7.8125) {

    // Count grain areas large enough to be counted
    int NumGrainsAboveThreshold = 0, AreaGrainsAboveThreshold = 0;
    int MinGrainSize =
        std::round(MinGrainSize_microns / (deltax * deltax * pow(10, 12))); // convert area to value in cells
    for (int n = 0; n < NumberOfGrains; n++) {
        if (GrainAreas[n] >= MinGrainSize) {
            AreaGrainsAboveThreshold += GrainAreas[n];
            NumGrainsAboveThreshold++;
        }
    }

    // What fraction of the cross-sectional area consists of grains large enough to be counted?
    double FractAreaAboveThreshold = DivideCast<double>(AreaGrainsAboveThreshold, CrossSectionSize);
    std::cout << "Area fraction of grains too small to include in statistics ( < " << MinGrainSize
              << " cells in area or less): " << 1 - FractAreaAboveThreshold << std::endl;

    // What's the average area for the grains large enough to be counted?
    double AvgAreaAboveThreshold =
        DivideCast<double>(CrossSectionSize * FractAreaAboveThreshold, NumGrainsAboveThreshold);
    std::cout << "Average grain area (in square microns): " << AvgAreaAboveThreshold * deltax * deltax * pow(10, 12)
              << std::endl;
    QoIs << "Average grain area (in square microns): " << AvgAreaAboveThreshold * deltax * deltax * pow(10, 12)
         << std::endl;

    // Print area of each grain to a file, as a fraction of the total area filled by grains large enough to be
    // considered
    std::string AreasFile = BaseFileName + "_" + ThisCrossSectionPlane + "_Areas.txt";
    std::ofstream Areas;
    Areas.open(AreasFile);
    for (int n = 0; n < NumberOfGrains; n++) {
        if (GrainAreas[n] >= MinGrainSize) {
            double ThisGrainArea = DivideCast<double>(GrainAreas[n], AreaGrainsAboveThreshold);
            Areas << ThisGrainArea << std::endl;
        }
    }
    Areas.close();
}

// Helper function for bimodal analysis of the grains in the specified cross-section
void AnalyzeCrossSection_Bimodal(std::ofstream &QoIs, std::string BaseFileName, std::string ThisCrossSectionPlane,
                                 double deltax, int NumberOfGrains, int CrossSectionSize,
                                 std::vector<int> UniqueGrainIDs, std::vector<int> GrainAreas, int NumberOfOrientations,
                                 ViewF_H GrainUnitVector, ViewF_H GrainRGBValues, float MinGrainSize_microns = 7.8125,
                                 float SmallLargeCutoff_microns = 1500) {

    // Make list of grains < "MinGrainSize" cells ("too small to include in statistics"), larger than
    // SmallLargeCutoff_microns sq microns ("large"), and in between ("small")
    int MinGrainSize =
        std::round(MinGrainSize_microns / (deltax * deltax * pow(10, 12))); // convert area to value in cells
    int SmallLargeCutoff =
        std::round(SmallLargeCutoff_microns / (deltax * deltax * pow(10, 12))); // convert area to value in cells
    int NumSmallGrains = 0, NumLargeGrains = 0;
    int AreaTooSmallGrains = 0, AreaSmallGrains = 0, AreaLargeGrains = 0;
    for (int n = 0; n < NumberOfGrains; n++) {
        if (GrainAreas[n] < MinGrainSize) {
            AreaTooSmallGrains += GrainAreas[n];
        }
        else {
            if (GrainAreas[n] < SmallLargeCutoff) {
                AreaSmallGrains += GrainAreas[n];
                NumSmallGrains++;
            }
            else {
                AreaLargeGrains += GrainAreas[n];
                NumLargeGrains++;
            }
        }
    }

    // What fraction of the cross-sectional area consists of each type of grain?
    double FractAreaTooSmall = DivideCast<double>(AreaTooSmallGrains, CrossSectionSize);
    double FractAreaSmall = DivideCast<double>(AreaSmallGrains, CrossSectionSize);
    double FractAreaLarge = DivideCast<double>(AreaLargeGrains, CrossSectionSize);
    std::cout << "Area fraction of grains too small to include in statistics ( < " << MinGrainSize
              << " cells in area or less): " << FractAreaTooSmall << std::endl;
    std::cout << "Area fraction of grains smaller than 1500 sq microns: " << FractAreaSmall << std::endl;
    QoIs << "Area fraction of grains smaller than 1500 sq microns: " << FractAreaSmall << std::endl;
    std::cout << "Area fraction of grains greater than or equal to 1500 sq microns: " << FractAreaLarge << std::endl;

    // What's the average area for small and large grains?
    double AvgAreaSmall = DivideCast<double>(CrossSectionSize * FractAreaSmall, NumSmallGrains);
    double AvgAreaLarge = DivideCast<double>(CrossSectionSize * FractAreaLarge, NumLargeGrains);
    std::cout << "Average area (in square microns) for small grains: " << AvgAreaSmall * deltax * deltax * pow(10, 12)
              << std::endl;
    std::cout << "Average area (in square microns) for large grains: " << AvgAreaLarge * deltax * deltax * pow(10, 12)
              << std::endl;
    QoIs << "Average area (in square microns) for small grains: " << AvgAreaSmall * deltax * deltax * pow(10, 12)
         << std::endl;
    QoIs << "Average area (in square microns) for large grains: " << AvgAreaLarge * deltax * deltax * pow(10, 12)
         << std::endl;

    // Print area of each small grain to a file, and area of each large grain to a file
    std::string SmallAreasFile = BaseFileName + "_" + ThisCrossSectionPlane + "_SmallAreas.txt";
    std::string LargeAreasFile = BaseFileName + "_" + ThisCrossSectionPlane + "_LargeAreas.txt";
    std::ofstream SmallAreas, LargeAreas;
    SmallAreas.open(SmallAreasFile);
    LargeAreas.open(LargeAreasFile);
    for (int n = 0; n < NumberOfGrains; n++) {
        if (GrainAreas[n] >= MinGrainSize) {
            if (GrainAreas[n] < SmallLargeCutoff)
                SmallAreas << static_cast<double>(GrainAreas[n] * deltax * deltax * pow(10, 12)) << std::endl;
            else
                LargeAreas << static_cast<double>(GrainAreas[n] * deltax * deltax * pow(10, 12)) << std::endl;
        }
    }
    SmallAreas.close();
    LargeAreas.close();

    // Print misorientation relative to the X, Y, and Z directions for small and large grains, weighted by grain area
    // For large grains, calculate the ratio of green (101 orientations) to blue (111 orientations)
    std::string MisorientationFileSmallX =
        BaseFileName + "_" + ThisCrossSectionPlane + "_XMisorientationSmallAreas.txt";
    std::string MisorientationFileSmallY =
        BaseFileName + "_" + ThisCrossSectionPlane + "_YMisorientationSmallAreas.txt";
    std::string MisorientationFileSmallZ =
        BaseFileName + "_" + ThisCrossSectionPlane + "_ZMisorientationSmallAreas.txt";
    std::string MisorientationFileLargeX =
        BaseFileName + "_" + ThisCrossSectionPlane + "_XMisorientationLargeAreas.txt";
    std::string MisorientationFileLargeY =
        BaseFileName + "_" + ThisCrossSectionPlane + "_YMisorientationLargeAreas.txt";
    std::string MisorientationFileLargeZ =
        BaseFileName + "_" + ThisCrossSectionPlane + "_ZMisorientationLargeAreas.txt";
    std::string GreenBlueFreqFile = BaseFileName + "_" + ThisCrossSectionPlane + "_GreenBlueRatio.txt";
    std::ofstream MisorientationSmallX, MisorientationSmallY, MisorientationSmallZ, MisorientationLargeX,
        MisorientationLargeY, MisorientationLargeZ, GreenBlueFreq;
    MisorientationSmallX.open(MisorientationFileSmallX);
    MisorientationSmallY.open(MisorientationFileSmallY);
    MisorientationSmallZ.open(MisorientationFileSmallZ);
    MisorientationLargeX.open(MisorientationFileLargeX);
    MisorientationLargeY.open(MisorientationFileLargeY);
    MisorientationLargeZ.open(MisorientationFileLargeZ);
    GreenBlueFreq.open(GreenBlueFreqFile);

    ViewF_H GrainMisorientationX = MisorientationCalc(NumberOfOrientations, GrainUnitVector, 0);
    ViewF_H GrainMisorientationY = MisorientationCalc(NumberOfOrientations, GrainUnitVector, 1);
    ViewF_H GrainMisorientationZ = MisorientationCalc(NumberOfOrientations, GrainUnitVector, 2);
    float GrainMisorientation_small_sumx = 0, GrainMisorientation_small_sumy = 0, GrainMisorientation_small_sumz = 0;
    float GrainMisorientation_large_sumx = 0, GrainMisorientation_large_sumy = 0, GrainMisorientation_large_sumz = 0;
    float GreenBlueRatioSum = 0.0;
    for (int n = 0; n < NumberOfGrains; n++) {
        if (GrainAreas[n] >= MinGrainSize) {

            int ThisGrainOrientation = getGrainOrientation(UniqueGrainIDs[n], NumberOfOrientations);
            if (ThisGrainOrientation < 0)
                throw std::runtime_error(
                    "Error analyzing grain misorientations: GrainID = 0 grain is present in the XY cross-section");
            float ThisGrainMisorientationX = GrainMisorientationX(ThisGrainOrientation);
            float ThisGrainMisorientationY = GrainMisorientationY(ThisGrainOrientation);
            float ThisGrainMisorientationZ = GrainMisorientationZ(ThisGrainOrientation);

            if (GrainAreas[n] < SmallLargeCutoff) {
                GrainMisorientation_small_sumx += GrainAreas[n] * ThisGrainMisorientationX;
                GrainMisorientation_small_sumy += GrainAreas[n] * ThisGrainMisorientationY;
                GrainMisorientation_small_sumz += GrainAreas[n] * ThisGrainMisorientationZ;
                for (int nn = 0; nn < GrainAreas[n]; nn++) {
                    MisorientationSmallX << ThisGrainMisorientationX << std::endl;
                    MisorientationSmallY << ThisGrainMisorientationY << std::endl;
                    MisorientationSmallZ << ThisGrainMisorientationZ << std::endl;
                }
            }
            else {
                GrainMisorientation_large_sumx += GrainAreas[n] * ThisGrainMisorientationX;
                GrainMisorientation_large_sumy += GrainAreas[n] * ThisGrainMisorientationY;
                GrainMisorientation_large_sumz += GrainAreas[n] * ThisGrainMisorientationZ;
                float Green = GrainRGBValues(3 * ThisGrainOrientation + 1);
                float Blue = GrainRGBValues(3 * ThisGrainOrientation + 2);
                float GreenBlueRatio_ThisGrain = Green / (Green + Blue);
                GreenBlueFreq << GreenBlueRatio_ThisGrain << std::endl;
                GreenBlueRatioSum += GreenBlueRatio_ThisGrain;
                for (int nn = 0; nn < GrainAreas[n]; nn++) {
                    MisorientationLargeX << ThisGrainMisorientationX << std::endl;
                    MisorientationLargeY << ThisGrainMisorientationY << std::endl;
                    MisorientationLargeZ << ThisGrainMisorientationZ << std::endl;
                }
            }
        }
    }
    MisorientationSmallX.close();
    MisorientationSmallY.close();
    MisorientationSmallZ.close();
    MisorientationLargeX.close();
    MisorientationLargeY.close();
    MisorientationLargeZ.close();
    GreenBlueFreq.close();
    float AvgMisorientation_Small_X = DivideCast<float>(GrainMisorientation_small_sumx, AreaSmallGrains);
    float AvgMisorientation_Small_Y = DivideCast<float>(GrainMisorientation_small_sumy, AreaSmallGrains);
    float AvgMisorientation_Small_Z = DivideCast<float>(GrainMisorientation_small_sumz, AreaSmallGrains);
    float AvgMisorientation_Large_X = DivideCast<float>(GrainMisorientation_large_sumx, AreaLargeGrains);
    float AvgMisorientation_Large_Y = DivideCast<float>(GrainMisorientation_large_sumy, AreaLargeGrains);
    float AvgMisorientation_Large_Z = DivideCast<float>(GrainMisorientation_large_sumz, AreaLargeGrains);
    float AvgGreenBlueRatio = GreenBlueRatioSum / NumLargeGrains;
    std::cout << "Average misorientation for small grains relative to the X direction: " << AvgMisorientation_Small_X
              << std::endl;
    std::cout << "Average misorientation for small grains relative to the Y direction: " << AvgMisorientation_Small_Y
              << std::endl;
    std::cout << "Average misorientation for small grains relative to the Z direction: " << AvgMisorientation_Small_Z
              << std::endl;
    std::cout << "Average misorientation for large grains relative to the X direction: " << AvgMisorientation_Large_X
              << std::endl;
    std::cout << "Average misorientation for large grains relative to the Y direction: " << AvgMisorientation_Large_Y
              << std::endl;
    std::cout << "Average misorientation for large grains relative to the Z direction: " << AvgMisorientation_Large_Z
              << std::endl;
    std::cout << "Average 101:111 ratio for large grains relative to the Z direction: " << AvgGreenBlueRatio
              << std::endl;
    QoIs << "Average misorientation for small grains relative to the X direction: " << AvgMisorientation_Small_X
         << std::endl;
    QoIs << "Average misorientation for small grains relative to the Y direction: " << AvgMisorientation_Small_Y
         << std::endl;
    QoIs << "Average misorientation for small grains relative to the Z direction: " << AvgMisorientation_Small_Z
         << std::endl;
    QoIs << "Average misorientation for large grains relative to the X direction: " << AvgMisorientation_Large_X
         << std::endl;
    QoIs << "Average misorientation for large grains relative to the Y direction: " << AvgMisorientation_Large_Y
         << std::endl;
    QoIs << "Average misorientation for large grains relative to the Z direction: " << AvgMisorientation_Large_Z
         << std::endl;
    QoIs << "Average 101:111 ratio for large grains relative to the Z direction: " << AvgGreenBlueRatio << std::endl;
}

// Analysis of data for the speified cross-section(s)
void PrintCrossSectionData(int NumberOfCrossSections, std::string BaseFileName,
                           std::vector<std::string> CrossSectionPlane, std::vector<int> CrossSectionLocation, int nx,
                           int ny, int nz, int NumberOfOrientations, ViewI3D_H GrainID,
                           std::vector<bool> PrintSectionPF, std::vector<bool> PrintSectionIPF,
                           std::vector<bool> BimodalAnalysis, bool NewOrientationFormatYN, double deltax,
                           ViewF_H GrainUnitVector, ViewF_H GrainEulerAngles, ViewF_H GrainRGBValues,
                           std::vector<std::string> CSLabels) {

    // Open file of cross-section quantities of interest
    std::ofstream QoIs;
    std::string OutputFilenameQoIs = BaseFileName + "_QoI.txt";
    QoIs.open(OutputFilenameQoIs);

    // Loop over each cross-section specified in the analysis file (grain_analysis) or over the two default
    // cross-sections (grain_analysis_amb)
    for (int n = 0; n < NumberOfCrossSections; n++) {

        // Print cross-section label to file
        QoIs << CSLabels[n] << std::endl;

        // Print data for pyEBSD/MTEX
        std::string ThisCrossSectionPlane = CrossSectionPlane[n]; // Which kind of cross-section is this?
        int Index1Low = 0;
        int Index2Low = 0;
        int Index1High, Index2High; // Values depend on the cross-section axes: nx, ny, or nz
        std::string Plane;
        if (ThisCrossSectionPlane.find("XZ") != std::string::npos) {
            Index1High = nx;
            Index2High = nz;
            Plane = "XZ";
        }
        else if (ThisCrossSectionPlane.find("YZ") != std::string::npos) {
            Index1High = ny;
            Index2High = nz;
            Plane = "YZ";
        }
        else if (ThisCrossSectionPlane.find("XY") != std::string::npos) {
            Index1High = nx;
            Index2High = ny;
            Plane = "XY";
        }
        else
            throw std::runtime_error("Error: cross-section for analysis must be XZ, YZ, or XY");

        // Should pole figure data be printed for this cross-section?
        // Should inverse pole figure-mapping data be printed for this cross-section?
        if ((PrintSectionPF[n]) || (PrintSectionIPF[n])) {
            std::cout << "Printing cross-section data for cross-section " << ThisCrossSectionPlane << std::endl;
            std::string FNameIPF = BaseFileName + "-" + ThisCrossSectionPlane + "_IPFCrossSection.txt";

            std::ofstream GrainplotIPF;
            ViewI_H GOHistogram("GOHistogram", NumberOfOrientations);
            if (PrintSectionIPF[n]) {
                GrainplotIPF.open(FNameIPF);
                GrainplotIPF << std::fixed << std::setprecision(6);
            }
            int NucleatedGrainCells = 0;
            int UnmeltedCells = 0;
            int CrossSectionSize = (Index1High - Index1Low) * (Index2High - Index2Low);
            std::vector<int> CrossSectionGrainIDs(CrossSectionSize);
            int Counter = 0;
            for (int Index1 = Index1Low; Index1 < Index1High; Index1++) {
                for (int Index2 = Index2Low; Index2 < Index2High; Index2++) {
                    int Index3 = CrossSectionLocation[n];
                    // How do Index1, Index2, Index3 correspond to GrainID(Z loc, X loc, Yloc)?
                    int ZLoc, XLoc, YLoc;
                    if (Plane == "XY") {
                        XLoc = Index1;
                        YLoc = Index2;
                        ZLoc = Index3;
                    }
                    else if (Plane == "YZ") {
                        XLoc = Index3;
                        YLoc = Index1;
                        ZLoc = Index2;
                    }
                    else {
                        XLoc = Index1;
                        YLoc = Index3;
                        ZLoc = Index2;
                    }
                    int GOVal = (abs(GrainID(ZLoc, XLoc, YLoc)) - 1) % NumberOfOrientations;
                    // Count number of cells in this cross-section have GrainID < 0 (grains formed via nucleation)
                    if (GrainID(ZLoc, XLoc, YLoc) < 0)
                        NucleatedGrainCells++;
                    // Add this GrainID to the vector of GrainIDs for this cross-section only if it is not equal to 0
                    if (GrainID(ZLoc, XLoc, YLoc) == 0)
                        UnmeltedCells++;
                    else {
                        CrossSectionGrainIDs[Counter] = GrainID(ZLoc, XLoc, YLoc);
                        Counter++;
                    }
                    // If constructing pole figure data from these orientations, add this value to the frequency data
                    if (PrintSectionPF[n])
                        GOHistogram(GOVal)++;
                    if (PrintSectionIPF[n]) {
                        if (NewOrientationFormatYN) {
                            // The grain structure is phase "1" - any unindexed points (which are possible from regions
                            // that didn't undergo melting) are assigned phase "0"
                            if (GOVal == -1)
                                GrainplotIPF << "0 0 0 0 " << Index1 * deltax * pow(10, 6) << " "
                                             << Index2 * deltax * pow(10, 6) << std::endl;
                            else
                                GrainplotIPF << GrainEulerAngles(3 * GOVal) << " " << GrainEulerAngles(3 * GOVal + 1)
                                             << " " << GrainEulerAngles(3 * GOVal + 2) << " 1 "
                                             << Index1 * deltax * pow(10, 6) << " " << Index2 * deltax * pow(10, 6)
                                             << std::endl;
                        }
                        else {
                            GrainplotIPF << Index1 * deltax * pow(10, 6) << "," << Index2 * deltax * pow(10, 6) << ","
                                         << GOVal << std::endl;
                        }
                    }
                }
            }
            double AreaFractNucleatedGrains = DivideCast<double>(NucleatedGrainCells, CrossSectionSize);
            double AreaFractUnmelted = DivideCast<double>(UnmeltedCells, CrossSectionSize);
            std::cout << "The fraction of the cross-section that went unmelted (not assigned a GrainID) is "
                      << AreaFractUnmelted << std::endl;
            // Resize cross-section to exclude unmelted cells
            CrossSectionSize = Counter;
            CrossSectionGrainIDs.resize(CrossSectionSize);
            QoIs << "The fraction of grains in this cross-section formed via nucleation events is "
                 << AreaFractNucleatedGrains << std::endl;
            if (PrintSectionIPF[n])
                GrainplotIPF.close();
            if (PrintSectionPF[n]) {
                std::string FNamePF = BaseFileName + "-" + ThisCrossSectionPlane + "_PFCrossSection.txt";
                WritePoleFigureDataToFile(FNamePF, NumberOfOrientations, GrainEulerAngles, GOHistogram);
            }
            // Make list of unique grains and corresponding grain areas
            std::vector<int> UniqueGrainIDs = FindUniqueGrains(CrossSectionGrainIDs);
            int NumberOfGrains = UniqueGrainIDs.size();
            std::cout << "The number of grains in this cross-section is " << NumberOfGrains << std::endl;
            std::vector<int> GrainAreas(NumberOfGrains, 0);
            for (int i = 0; i < NumberOfGrains; i++) {
                for (int j = 0; j < CrossSectionSize; j++) {
                    if (UniqueGrainIDs[i] == CrossSectionGrainIDs[j])
                        GrainAreas[i]++;
                }
            }

            // Should these grains be analyzed as a single distribution of grain areas, or a bimodal distribution
            // (bimodal distribution option also includes printing of misorientation data)
            if (BimodalAnalysis[n])
                AnalyzeCrossSection_Bimodal(QoIs, BaseFileName, ThisCrossSectionPlane, deltax, NumberOfGrains,
                                            CrossSectionSize, UniqueGrainIDs, GrainAreas, NumberOfOrientations,
                                            GrainUnitVector, GrainRGBValues);
            else
                AnalyzeCrossSection_Unimodal(QoIs, BaseFileName, ThisCrossSectionPlane, deltax, NumberOfGrains,
                                             CrossSectionSize, GrainAreas);
        }
    }
    QoIs.close();
}

//*****************************************************************************/
void WritePoleFigureDataToFile(std::string Filename, int NumberOfOrientations, ViewF_H GrainEulerAngles,
                               ViewI_H GOHistogram) {

    // Using new format, write pole figure data to "Filename"
    std::ofstream GrainplotPF;
    GrainplotPF.open(Filename);
    GrainplotPF << "% MTEX ODF" << std::endl;
    GrainplotPF << "% crystal symmetry: \"m-3m\"" << std::endl;
    GrainplotPF << "% specimen symmetry: \"43\"" << std::endl;
    GrainplotPF << "% phi1    Phi     phi2    value" << std::endl;
    GrainplotPF << std::fixed << std::setprecision(6);
    for (int i = 0; i < NumberOfOrientations; i++) {
        GrainplotPF << GrainEulerAngles(3 * i) << " " << GrainEulerAngles(3 * i + 1) << " "
                    << GrainEulerAngles(3 * i + 2) << " " << (float)(GOHistogram(i)) << std::endl;
    }
    GrainplotPF.close();
}

//*****************************************************************************/
void WritePoleFigureDataToFile_OldFormat(std::string Filename, int NumberOfOrientations, ViewI_H GOHistogram) {

    // Using old format, write histogram data (used by a second post-processing script to construct pole figures) to
    // "Filename"
    std::ofstream MTEXPlot;
    MTEXPlot.open(Filename);
    for (int i = 0; i < NumberOfOrientations; i++) {
        MTEXPlot << GOHistogram[i] << std::endl;
    }
    MTEXPlot.close();
}

//*****************************************************************************/
void PrintExaConstitRVEData(int NumberOfRVEs, std::string BaseFileName, int, int, int, double deltax, ViewI3D_H GrainID,
                            std::vector<int> XLow_RVE, std::vector<int> XHigh_RVE, std::vector<int> YLow_RVE,
                            std::vector<int> YHigh_RVE, std::vector<int> ZLow_RVE, std::vector<int> ZHigh_RVE) {

    // Loop over each RVE specified in the file "AnalysisOutputs.txt"
    for (int n = 0; n < NumberOfRVEs; n++) {
        // Print data for ExaConstit
        std::string FNameE = BaseFileName + std::to_string(n) + "_ExaConstit.csv";
        std::cout << "RVE number " << n + 1 << " with X coordinates " << XLow_RVE[n] << "," << XHigh_RVE[n]
                  << "; Y coordinates " << YLow_RVE[n] << "," << YHigh_RVE[n] << "; Z coordinates " << ZLow_RVE[n]
                  << "," << ZHigh_RVE[n] << " being printed to file " << FNameE << " for ExaConstit" << std::endl;
        std::ofstream GrainplotE;
        GrainplotE.open(FNameE);
        GrainplotE << "Coordinates are in CA units, 1 cell = " << deltax << " m. Data is cell-centered. Origin at "
                   << XLow_RVE[n] << "," << YLow_RVE[n] << "," << ZLow_RVE[n] << " , domain size is "
                   << XHigh_RVE[n] - XLow_RVE[n] + 1 << " by " << YHigh_RVE[n] - YLow_RVE[n] + 1 << " by "
                   << ZHigh_RVE[n] - ZLow_RVE[n] + 1 << " cells" << std::endl;
        GrainplotE << "X coord, Y coord, Z coord, Grain ID" << std::endl;
        int NucleatedGrainCells = 0;
        for (int k = ZLow_RVE[n]; k <= ZHigh_RVE[n]; k++) {
            for (int j = YLow_RVE[n]; j <= YHigh_RVE[n]; j++) {
                for (int i = XLow_RVE[n]; i <= XHigh_RVE[n]; i++) {
                    GrainplotE << i << "," << j << "," << k << "," << GrainID(k, i, j) << std::endl;
                    if (GrainID(k, i, j) < 0)
                        NucleatedGrainCells++;
                }
            }
        }
        GrainplotE.close();
        int RVESize = (XHigh_RVE[n] - XLow_RVE[n]) * (YHigh_RVE[n] - YLow_RVE[n]) * (ZHigh_RVE[n] - ZLow_RVE[n]);
        double RVEFractNucleatedGrains = DivideCast<double>(NucleatedGrainCells, RVESize);
        std::cout << "The fraction of grains formed via nucleation events in this RVE is " << RVEFractNucleatedGrains
                  << std::endl;
    }
}
