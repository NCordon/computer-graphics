#include "camara.hpp"

static const double udesp = 0.5;
static const double urot = M_PI/30.0;
static const double dmin = 1;
static const double porc = 1;

ViewFrustum::ViewFrustum(){
    persp = false;
    left = bottom = -1;
    right = top = 1;
    near = 0;
    far = 2;

    /*
    Matriz4f traslacion = MAT_Traslacion(
        -0.5*(left + right),
        -0.5*(top + bottom),
        -0.5*(far + near)
    );

    Matriz4f escalado = MAT_Escalado(
         2/(right - left),
         2/(top - bottom),
        -2/(far - near)
    );

    matrizProy = escalado * traslacion;
    */
    matrizProy = MAT_Ortografica(left, right, bottom, top, near, far);
}

ViewFrustum::ViewFrustum( float hfovy, float aspect, float zNear, float zFar ){
    persp = true;
    top = zNear * tan(0.5 * hfovy * M_PI/180.0);
    right = top * aspect;
    bottom = -top;
    left = -right;

    matrizProy = MAT_Frustum(left, right, bottom, top, zNear, zFar);
}


MarcoCoorVista::MarcoCoorVista(){
    org = Tupla3f(0,0,0);
    pvup = Tupla3f(0,1,0);
    eje[0] = Tupla3f(1,0,0);
    eje[1] = Tupla3f(0,0,1);
    eje[2] = Tupla3f(0,1,0);

    for (int i=0; i<4; i++)
        for(int j=0; j<4; j++)
            if(i==j)
                matrizML(i,j) = matrizLM(i,j) = 1;
            else
                matrizML(i,j) = matrizLM(i,j) = 0;
}

MarcoCoorVista::MarcoCoorVista( const Tupla3f &pfoco, const Tupla3f &paten, const Tupla3f &pvup){
    org = pfoco;
    this->pvup = pvup;

    eje[2] = (pfoco - paten).normalized();
    eje[0] = pvup.cross(eje[2]).normalized();  // eje Z apunta en la dir. contraria a la camara
    eje[1] = eje[2].cross( eje[0] );                     // eje Y perpendicular a los otros dos.

    Matriz4f  traslacion = MAT_Traslacion( -pfoco[X], -pfoco[Y], -pfoco[Z] ),
              rot = MAT_Ident() ; // matriz de cambio de base mundo --> camara

    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
              rot(i,j) = eje[i](j);

    matrizML = rot * traslacion;
}

Camara::Camara(){}


CamaraInteractiva::CamaraInteractiva(){
    examinar = false;
    aten = Tupla3f(0,0,0);
    long_rot = 0;
    lati_rot = 0;
    dist = 1;
}


void CamaraInteractiva::moverHV( int nh, int nv ){
    if(examinar){
        double grad_long = nh*urot * 180 / M_PI;
        double grad_lati = nv*urot * 180 / M_PI;

        long_rot += nh*urot;
        lati_rot += nv*urot;
        Matriz4f rot_long = MAT_Rotacion(grad_long,0,1,0);
        Matriz4f rot_lati = MAT_Rotacion(grad_lati,1,0,0);
        mcv.org = aten + rot_lati * rot_long * (mcv.org-aten);
        mcv.eje[1] = rot_lati * rot_long * mcv.eje[1];

        mcv = MarcoCoorVista( mcv.org, aten, mcv.eje[1]);
    }
    else{
        Tupla3f desp_x = mcv.eje[0] * ((float) nh*udesp),
                desp_y = mcv.eje[1] * ((float) nv*udesp);

        Tupla3f desp = desp_x + desp_y;
        mcv.matrizML = mcv.matrizML * MAT_Traslacion( -desp(X), -desp(Y), -desp(Z) );
    }
}

void CamaraInteractiva::desplaZ( int nz ){
    if(examinar){
        dist = sqrt((mcv.org - aten).dot(mcv.org - aten));
        Tupla3f desp = (float)(dist - dmin - (dist - dmin)*(1.0 - nz*porc/100.0)) * mcv.eje[2];
        mcv.org = mcv.org - desp;

        mcv.matrizML = mcv.matrizML * MAT_Traslacion( desp(X), desp(Y), desp(Z) );
    }
    else{
        Tupla3f desp = ((float)(nz * udesp))*mcv.eje[2];
        mcv.org = mcv.org - desp;

        mcv.matrizML = mcv.matrizML * MAT_Traslacion( desp(X), desp(Y), desp(Z) );
    }
}

void Camara::fijarMVPOpenGL(){
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glMultMatrixf(mcv.matrizML);

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glMultMatrixf(vf.matrizProy);

    glutPostRedisplay();
}

void CamaraInteractiva::fijarAten(const Tupla3f & nuevoAten){
    aten = nuevoAten;

    //La primera vez
    if (!examinar){
        examinar = true;
        desplaZ(75);
    }
    mcv = MarcoCoorVista(mcv.org, aten, Tupla3f(0,1,0));
}
