#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <cmath>

using namespace std;

class RAM_Access{

    public:

    int x;
    int y;
    int address;

    RAM_Access(){}
    RAM_Access(int a, int b){
        x = a;
        y = b;
    }

    RAM_Access(const RAM_Access &old){
        x = old.x;
        y = old.y;
    }

};

int RAM_SIZE, mode;

long long SRAM_access = 0, DRAM_access = 0;

long long ram_load = 0;
long ten2eight = 100000000;
long DRAM_access_m = 0;
/*
 *  VR Projection
 */
double PI = 3.14159265358979323846;

int w, h;
int fw, fh;
int fovX, fovY;
double hp, ht;

double toRadian(double a){
    return a / 180.0 * PI;
}

int nearestNeighbor(double num){

  int res = (int)(num + 0.5);

  return res;
}

void spherical2cartesian(double the, double phi, double result[3]){

    double x = sin(phi) * cos(the);
    double y = sin(phi) * sin(the);
    double z = cos(phi);


    result[0] = x;
    result[1] = y;
    result[2] = z;

}

void spherical2coordinates(double the, double phi, double result[2]){

    double i,j;

    if(the > PI){
        i = (the - PI) / 2.0 / PI * w;
    }
    else{
        i = (PI + the) / 2.0 / PI * w;
    }

    j = phi /  PI * h;

    result [0] = i;
    result [1] = j;
}

void cartesian2coordinates(double x, double y, double z, double result[2]){

    double the;

    if(x != 0) {
        the = atan2(y, x);

    } else {
        the = toRadian(90.0);
    }

    double phi = acos(z);
    spherical2coordinates(the, phi, result);
}

//New approach
void coordinates2spherical(double i, double j, double result[2]){
    double theta, phi;

    if (i >= w / 2.0){

        theta = (2 * i * PI / w) - PI;

    }
    else{

        theta = (2 * i * PI / w) + PI;

    }
    phi = j * PI / h;
    //printf("theta: %lf , phi: %lf\n", theta,phi);
    result[0] = theta;
    result[1] = phi;
}


void cartesian2coordinates_inverse(double x, double y, double z, double result[2]){

    double the;
    // pay atentions to atan2 vs atan
    if (x != 0){

        the = atan2(y, x);

    }
    else{

        the = toRadian(90.0);

    }

    double phi = acos(z);

    the = the / PI * 180.0;
    phi = phi / PI * 180.0;

    if(the >= -fovX/2.0 && the <= fovX/2 && phi >= 90 -fovY/2.0 && phi <= 90 +fovY/2.0){

        result[0] = (the + fovX/2.0)* fw /fovX;
        result[1] = (phi -  90  + fovY/2.0) * fh/ fovY;
    }
    else{

        result[0] = 0;
        result[1] = 0;
    }
}


void coordinates2cartesian(double i, double j, double result[3]){

    double angles [] = {0.0, 0.0};

    coordinates2spherical(i, j, angles);

    spherical2cartesian(angles[0],angles[1],result);

}

void matrixMultiplication(double* vector, double matrix[3][3], double res[3]) {

    for (int i=0; i<3; i++) {
        for (int j=0; j<3; j++) {

            res[i] += matrix[i][j] * vector[j];
        }
    }

}

void loadSRAM(RAM_Access ***dram, int** sram, int i, int j,  int mode){

    int m, n;
    int pattern;

    if(mode == 0){
        m = i;
        n = j;
        pattern = 2;
    }
    else if(mode == 2){
        m = i;
        n = 0;
        pattern = 2;
    }

    switch (pattern) {

        case 0:
            // load by square patches


        case 2:
            // load by lines sequentially
            for (int s = 0; s < RAM_SIZE; s++) {
                sram[m][n] = (int)&dram[m][n];
//                printf("sram: %d, dram: %d\n", sram[m][n], (int)&dram[m][n]);
                m++;
                if (m > w - 1){
                    m = 0;
                    n++;
                }
                if (n > h - 1) {
                    break;
                }

                DRAM_access++;
                if(DRAM_access == ten2eight){
                    DRAM_access_m++;
                    DRAM_access = 0;
                }
            }
            break;
    }

}

//bool sram_contains(int i, int j, int** sram){
//
//    return sram[i][j] != 0;
//}
//
//bool check_sram(int i, int j, int** sram){
//
//    return sram_contains(i, j, sram);
//}

int main(int argc, char** argv) {

    // parameter for simulator
    // number of pixels sram could store
    // 627 is in KB
    RAM_SIZE = 627 * 1024 / 3; //214016
    mode = 0;

    // input image size
    w = 3840;
    h = 2160;

    // parameters for FOV
    fw = 1174;
    fh = 1080;
    fovX = 110;
    fovY = 90;
    hp = 5;
    ht = 0;

    int **SRAM = (int**)calloc(w, sizeof(int*));

    for(int i = 0; i < w; i++){

        SRAM[i] = (int*)calloc(h, sizeof(int));
    }

    RAM_Access ***DRAM = (RAM_Access***)calloc(w, sizeof(RAM_Access**));

    DRAM = new RAM_Access **[w];

    for(int i = 0; i < w; i++){

        DRAM[i] = new RAM_Access*[h];

        for(int j = 0; j < h; j++){

            DRAM[i][j] = new RAM_Access(i, j);
//            printf("i: %d, j:%d\n", dram[i][j]->x, dram[i][j]->y);
        }
    }

    // convert to radian
    double htr = toRadian(ht);
    double hpr = toRadian(hp);

    // rotation matrices
    double rot_y [3][3] = {
            {cos(hpr), 0, -sin(hpr)},
            {0, 1, 0},
            {sin(hpr), 0, cos(hpr)}
    };

    double rot_z [3][3] = {
            {cos(htr), sin(htr), 0},
            {-sin(htr), cos(htr), 0},
            {0, 0, 1}
    };

    int a = 0, b = 0;

    if(mode == 0) {

        double rot_y_inverse[3][3] = {
                {cos(hpr),  0, sin(hpr)},
                {0,         1, 0},
                {-sin(hpr), 0, cos(hpr)}};

        double rot_z_inverse[3][3] = {
                {cos(htr), -sin(htr), 0},
                {sin(htr), cos(htr),  0},
                {0,        0,         1}};

        //border on the input frame that map to output pixels

        double maxX = -INFINITY;
        double minX = INFINITY;
        double maxY = -INFINITY;
        double minY = INFINITY;

        double jT = -fovX / 2.0;
        double jR = fovX / 2.0;
        double jB = -fovX / 2.0;
        double jL = 360 - fovX / 2.0;
        double iT = 90 - fovY / 2.0;
        double iR = 90 - fovY / 2.0;
        double iB = 90 + fovY / 2.0;
        double iL = 90 - fovY / 2.0;

        double i = 0.0;
        double j = 0.0;

        for(int k = 0; k < 2*(fh+fw); k++){

            //Top
            if (k < fw){
                i = iT;
                j = jT;
                jT +=  fovX * 1.0 / fw;
            }

            //Right
            if ((k >= fw) && (k < fw+fh)){
                i = iR;
                j = jR;
                iR += fovY * 1.0 / fh;
            }

            //Bottom
            if ((k >= fw+fh) && (k < 2*fw+fh)){
                i = iB;
                j = jB;
                jB +=  fovX * 1.0 / fw;
            }

            //Left
            if ((k >= 2*fw+fh) && (k < 2*(fw+fh))){
                i = iL;
                j = jL;
                iL += fovY * 1.0 / fh;
            }

            // rotation along y axis
            double p1[] = {0.0, 0.0, 0.0};
            spherical2cartesian(toRadian((j < 0)? (j + 360): j), toRadian((i < 0) ? (i + 180) : i), p1);

            double p2[] = {0.0, 0.0, 0.0};
            matrixMultiplication(p1, rot_y, p2);

            // rotate along z axis
            double p3[] = {0.0, 0.0, 0.0};
            matrixMultiplication(p2, rot_z, p3);

            double res[] = {0.0, 0.0};

            // convert 3D catesian to 2D coordinates
            cartesian2coordinates(p3[0], p3[1], p3[2], res);

            if (b >= fh) break;

            if (minX > res[0]) minX = res[0];
            if (maxX < res[0]) maxX = res[0];
            if (minY > res[1]) minY = res[1];
            if (maxY < res[1]) maxY = res[1];


        }

        if (hp <= -45 || hp >= 315){

            maxY = h;
            maxX = w;
            minX = 0.0;

        }
        if(hp >= 45) {

            minY = 0.0;
            maxX = w;
            minX = 0.0;

        }

        printf("Max: %lf %lf, Min: %lf %lf\n", maxX, maxY, minX, minY);
        //for input pixel in the output range, calculate the outpout cordinnates
        int x , y;

        for (y = 0; y < h; y++){
            for (x = 0; x < w; x++){

                //if pixel map to output get input index
                if (x <= maxX && x >= minX && y <= maxY && y >= minY){

//                    double cartesian []  ={0.0, 0.0, 0.0};
//                    coordinates2cartesian(x, y, cartesian);
//
//                    double p1[] = {0.0, 0.0, 0.0};
//                    matrixMultiplication(cartesian, rot_z_inverse , p1);
//
//                    // rotate along z axis
//                    double p2[] = {0.0, 0.0, 0.0};
//                    matrixMultiplication( p1, rot_y_inverse, p2);
//
//
//                    double res[] = {0.0,0.0};
//                    cartesian2coordinates_inverse(p2[0], p2[1], p2[2], res);
////                    fov.at<Vec3b>(nearestNeighbor(res[1]), nearestNeighbor(res[0])) = image.at<Vec3b>(y,x);
//
//                    int temp_x = nearestNeighbor(res[0]);
//                    int temp_y = nearestNeighbor(res[1]);

                    if (SRAM[x][y] == 0) {
                        for(int a = 0; a < w; a++){
                            for(int b = 0; b < h; b++){
                                SRAM[a][b] = 0;
                            }
                        }
//                        loadSRAM(DRAM, SRAM, x, y, mode);
                        int m, n;
                        int pattern;

                        if(mode == 0){
                            m = x;
                            n = y;
                            pattern = 2;
                        }
                        else if(mode == 2){
                            m = x;
                            n = 0;
                            pattern = 2;
                        }

                        switch (pattern) {

                            case 0:
                                // load by square patches


                            case 2:
                                // load by lines sequentially
                                for (int s = 0; s < RAM_SIZE; s++) {
                                    SRAM[m][n] = (int)&DRAM[m][n];
//                printf("sram: %d, dram: %d\n", sram[m][n], (int)&dram[m][n]);
                                    m++;
                                    if (m > w - 1){
                                        m = 0;
                                        n++;
                                    }
                                    if (n > h - 1) {
                                        break;
                                    }

                                    DRAM_access++;
                                    if(DRAM_access == ten2eight){
                                        DRAM_access_m++;
                                        DRAM_access = 0;
                                    }
                                }
                                break;
                        }
                        ram_load++;
                    }
                    SRAM_access++;
//                    printf("x: %d, y :%d\n",x, y);

                }
            }
        }
    }

    else {

        //    int pixel_count = 0;
        // default head orientation is 0,90
        for (double i = 90 - fovY / 2.0; i < 90 + fovY / 2.0; i += fovY * 1.0 / fh, b++) {
            for (double j = -fovX / 2.0; j < fovX / 2.0; j += fovX * 1.0 / fw, a++) {

                double p1[] = {0.0, 0.0, 0.0};
                spherical2cartesian(toRadian((j < 0) ? j + 360 : j), toRadian((i < 0) ? (i + 180) : i), p1);
                // rotation along y axis
                double p2[] = {0.0, 0.0, 0.0};
                matrixMultiplication(p1, rot_y, p2);

                // rotate along z axis
                double p3[] = {0.0, 0.0, 0.0};
                matrixMultiplication(p2, rot_z, p3);

                double res[] = {0.0, 0.0};

                // convert 3D catesian to 2D coordinates
                cartesian2coordinates(p3[0], p3[1], p3[2], res);

                if (b >= fh || a >= fw) {
                    break;
                }

                int temp_x = nearestNeighbor(res[0]);
                int temp_y = nearestNeighbor(res[1]);
//                printf("sram: %d\n" ,SRAM[temp_x][temp_y]);
                if (SRAM[temp_x][temp_y] == 0) {
                    for(int a = 0; a < w; a++){
                        for(int b = 0; b < h; b++){
                            SRAM[a][b] = 0;
                        }
                    }
//                    loadSRAM(DRAM, SRAM, temp_x, temp_y, 2);
//                    printf("RAM load: %d\n", ram_load);

                    int m, n;
                    int pattern;

                    if(mode == 0){
                        m = temp_x;
                        n = temp_y;
                        pattern = 2;
                    }
                    else if(mode == 2){
                        m = 0;
                        n = temp_y;
                        pattern = 2;
                    }

                    switch (pattern) {

                        case 0:
                            // load by square patches


                        case 2:
                            // load by lines sequentially
                            for (int s = 0; s < RAM_SIZE; s++) {
                                SRAM[m][n] = (int)&DRAM[m][n];
//                                printf("sram: %d, dram: %d\n", SRAM[temp_x][temp_y], (int)&DRAM[m][n]);
                                m++;
                                if (m > w - 1){
                                    m = 0;
                                    n++;
                                }
                                if (n > h - 1) {
                                    break;
                                }

                                DRAM_access++;
                                if(DRAM_access == ten2eight){
                                    DRAM_access_m++;
                                    DRAM_access = 0;
                                }
                            }
                            break;
                    }

//                    printf("value: %d\n", SRAM[temp_x][temp_y]);
                    ram_load++;
                }
                SRAM_access++;

                //            printf("Pixel: %d\n", pixel_count++);
            }
            a = 0;
        }
    }

    printf("DRAM Access: %dE+08 %d\n", DRAM_access_m, DRAM_access);
    printf("SRAM Access: %d\n", SRAM_access);
    printf("SRAM Load: %d\n", ram_load);

    return 0;
}
