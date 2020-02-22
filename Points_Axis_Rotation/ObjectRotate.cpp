#include <bits/stdc++.h>
#include <time.h>
#include <sys/time.h>
#include <omp.h>
using namespace std;

#define eps 1e-5
#define pr(...) dbs(#__VA_ARGS__, __VA_ARGS__);
template <class T> void dbs(string str, T t) { cerr << str << " : " << t << "\n"; }
template <class T, class... S> void dbs(string str, T t, S... s) { int idx = str.find(','); cerr << str.substr(0, idx) << " : " << t << ", "; dbs(str.substr(idx + 1), s...); }

/* Triplet holds coordinate points or vector components */
class triplet {
 public:
    float x, y, z;

    triplet() = default;
    triplet(float x1, float y1, float z1) : x(x1), y(y1), z(z1) {};

    float magnitude(){
        return sqrt(x*x + y*y + z*z);
    }

    triplet unit_vector(){
        float mag = magnitude();
        return triplet(x/mag, y/mag, z/mag);
    }
};
/* Helps Printing Triplet DataType as: x, y, z */
ostream& operator <<(ostream& os, const triplet& p) {
    return os << p.x << ", " << p.y << ", " << p.z;
}

/* Returns Current CPU Time  */
double getWallTime()
{
    struct timeval time;
    if(gettimeofday(&time, NULL)) return 0;
    double wallTime = (double)time.tv_sec + (double)time.tv_usec * 0.000001;
    return wallTime;
}

/* Extracts point P and Q of Axis of Rotation */
/* Expects FileLine1 to be strictly of format - "P(45,-13,73) Q(76,12,73)" */
void Extract_Axis(char* axis_filename, triplet* P, triplet* Q)
{
    FILE* axis_fp = fopen(axis_filename, "r");
    fscanf(axis_fp, "%*c(%f%*c%f%*c%f) %*c(%f%*c%f%*c%f)", &(P->x), &(P->y), &(P->z), &(Q->x), &(Q->y), &(Q->z));
    fclose(axis_fp);
}

/* Extracts Points of the Object Line by Line */
/* Each FileLine should have three space seperated points */
vector<triplet> Extract_Points(char* object_filename)
{   
    ifstream object_filestream; 
    object_filestream.open(object_filename);

    float x, y, z;
    vector<triplet> object;
    while(object_filestream >> x >> y >> z){
        object.push_back(triplet(x, y, z));
    }

    object_filestream.close();

    return object;
}

void multiply_4x4_matrix_vector(float matrix[4][4], vector<float>& initVec)
{
    assert(initVec.size() == 4);
    vector<float> finalVec(4, 0);
    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 4; j++){
            finalVec[i] += matrix[i][j] * initVec[j];
        }
    }
    initVec = finalVec;
    finalVec.clear();
}

float Translate_Matrix[4][4] = {}; float InvTranslate_Matrix[4][4] = {};
float   RotateX_Matrix[4][4] = {}; float   InvRotateX_Matrix[4][4] = {};
float   RotateY_Matrix[4][4] = {}; float   InvRotateY_Matrix[4][4] = {};
float   RotateZ_Matrix[4][4] = {};

/* Initialising the Operation Matrices beforehand to enable reuse as much as possible */
/* Takes as Argument 1.) First Point of the Axis of Rotation
                     2.) The Unit Vector along the Axis
                     3.) Angle of Rotation                      */
void init(triplet P, triplet U_vector, float angleOfRotation)
{
    // Create Translation Matrix
    for(int i = 0; i < 4; i++) Translate_Matrix[i][i] = 1;
    Translate_Matrix[0][3] = -P.x;
    Translate_Matrix[1][3] = -P.y;
    Translate_Matrix[2][3] = -P.z;

    // Create Inverse Translation Matrix
    for(int i = 0; i < 4; i++) InvTranslate_Matrix[i][i] = 1;
    InvTranslate_Matrix[0][3] = P.x;
    InvTranslate_Matrix[1][3] = P.y;
    InvTranslate_Matrix[2][3] = P.z;

    float projectionOnYZPlane = sqrt(U_vector.y*U_vector.y + U_vector.z*U_vector.z);

    // Create RotateX and InvRotateX Matrices
    RotateX_Matrix[0][0] =  1;
    RotateX_Matrix[3][3] =  1;
    InvRotateX_Matrix[0][0] =  1;
    InvRotateX_Matrix[3][3] =  1;

    if (projectionOnYZPlane < eps) {
        RotateX_Matrix[1][1] =
        RotateX_Matrix[2][2] = 1;
        RotateX_Matrix[1][2] =
        RotateX_Matrix[2][1] = 0;
        InvRotateX_Matrix[1][1] =
        InvRotateX_Matrix[2][2] = 1;
        InvRotateX_Matrix[1][2] =
        InvRotateX_Matrix[2][1] = 0;
    } else {
        RotateX_Matrix[1][1] =  U_vector.z/projectionOnYZPlane;
        RotateX_Matrix[2][2] =  U_vector.z/projectionOnYZPlane;
        RotateX_Matrix[1][2] = -U_vector.y/projectionOnYZPlane;
        RotateX_Matrix[2][1] =  U_vector.y/projectionOnYZPlane;
        InvRotateX_Matrix[1][1] =  U_vector.z/projectionOnYZPlane;
        InvRotateX_Matrix[2][2] =  U_vector.z/projectionOnYZPlane;
        InvRotateX_Matrix[1][2] =  U_vector.y/projectionOnYZPlane;
        InvRotateX_Matrix[2][1] = -U_vector.y/projectionOnYZPlane;
    }

    // Create RotateY Matrix
    RotateY_Matrix[0][0] =  projectionOnYZPlane;
    RotateY_Matrix[1][1] =  1;
    RotateY_Matrix[2][2] =  projectionOnYZPlane;
    RotateY_Matrix[3][3] =  1;
    RotateY_Matrix[0][2] = -U_vector.x;
    RotateY_Matrix[2][0] =  U_vector.x;

    // Create InvRotateY Matrix
    InvRotateY_Matrix[0][0] =  projectionOnYZPlane;
    InvRotateY_Matrix[1][1] =  1;
    InvRotateY_Matrix[2][2] =  projectionOnYZPlane;
    InvRotateY_Matrix[3][3] =  1;
    InvRotateY_Matrix[0][2] =  U_vector.x;
    InvRotateY_Matrix[2][0] = -U_vector.x;

    // Create RotateZ Matrix
    RotateZ_Matrix[0][0] =  cos(angleOfRotation);
    RotateZ_Matrix[1][1] =  cos(angleOfRotation);
    RotateZ_Matrix[2][2] =  1;
    RotateZ_Matrix[3][3] =  1;
    RotateZ_Matrix[0][1] = -sin(angleOfRotation);
    RotateZ_Matrix[1][0] =  sin(angleOfRotation);
}

void Translate_3DSpace(vector<float>& point)
{
    multiply_4x4_matrix_vector(Translate_Matrix, point);
}

void TranslateInv_3DSpace(vector<float>& point)
{
    multiply_4x4_matrix_vector(InvTranslate_Matrix, point);
}

void Rotate_AboutX(vector<float>& point)
{
    multiply_4x4_matrix_vector(RotateX_Matrix, point);
}

void RotateInv_AboutX(vector<float>& point)
{
    multiply_4x4_matrix_vector(InvRotateX_Matrix, point);
}

void Rotate_AboutY(vector<float>& point)
{
    multiply_4x4_matrix_vector(RotateY_Matrix, point);
}

void RotateInv_AboutY(vector<float>& point)
{
    multiply_4x4_matrix_vector(InvRotateY_Matrix, point);
}

void Rotate_AboutZ(vector<float>& point)
{
    multiply_4x4_matrix_vector(RotateZ_Matrix, point);
}

int main(int argc, char* argv[])
{
    assert(argc >= 5);

    int numThreads; sscanf(argv[1], "%d", &numThreads);
    char* axis_filename = argv[2];
    char* object_filename = argv[3];
    float angleOfRotation; sscanf(argv[4], "%f", &angleOfRotation);
    angleOfRotation = (angleOfRotation/180.0) * M_PI;

    triplet P_point, Q_point;
    Extract_Axis(axis_filename, &P_point, &Q_point);
    triplet U_vector = triplet(Q_point.x - P_point.x, 
                               Q_point.y - P_point.y, 
                               Q_point.z - P_point.z).unit_vector();
    // triplet A_vector(Q_point.x - P_point.x, Q_point.y - P_point.y, Q_point.z - P_point.z);
    // triplet U_vector = A_vector.unit_vector();

    vector<triplet> objectpts_org = Extract_Points(object_filename);
    int n = objectpts_org.size();

    init(P_point, U_vector, angleOfRotation);

    vector<triplet> objectpts_rot(n);

    const double startTime = getWallTime();
    #pragma omp parallel for num_threads(numThreads)
    for(int i = 0; i < n; i++)
    {
        vector<float> point = {objectpts_org[i].x, objectpts_org[i].y, objectpts_org[i].z, 1};

        Translate_3DSpace(point);           // Translate to P
        Rotate_AboutX(point);               // Rotate About X
        Rotate_AboutY(point);               // Rotate About Y
        Rotate_AboutZ(point);               // Rotate About Z
        RotateInv_AboutY(point);            // Inverse Rotate About Y
        RotateInv_AboutX(point);            // Inverse Rotate About X
        TranslateInv_3DSpace(point);        // Inverse Translate From P

        // sprintf(tmp, "%.3f %.3f %.3f", point[0], point[1], point[2]);
        // sscanf(tmp, "%f %f %f", &point[0], &point[1], &point[2]);
        objectpts_rot[i] = triplet(point[0], point[1], point[2]);
    }
    const double stopTime = getWallTime();
    const double timeElapsed = stopTime - startTime;

    pr(n, numThreads, timeElapsed);

    for(int i = 0; i < n; i++) {
        printf("%.3f\t%.3f\t%.3f\n", objectpts_rot[i].x, objectpts_rot[i].y, objectpts_rot[i].z);
    }

    return 0;
}
