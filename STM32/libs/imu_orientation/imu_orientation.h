#ifndef IMU_ORIENTATION_H
#define IMU_ORIENTATION_H

#include <stdbool.h>

typedef struct {
    float roll_deg;
    float pitch_deg;
    float yaw_deg;   /* Relativo: deriva sem referencia externa */
} IMU_Attitude;

/* Inicializa a referencia zero a partir da media bruta da aceleracao (em g).
 * Exige modulo parado e |a| aproximadamente 1 g. Retorna false caso invalida.
 */
bool IMU_OrientationBegin(float mean_ax_g, float mean_ay_g, float mean_az_g);

/* Atualiza o filtro Mahony 6DoF com aceleracao BRUTA em g,
 * giroscopio CORRIGIDO pelo bias em graus/s, e dt real em segundos.
 */
bool IMU_OrientationUpdate(float ax_g, float ay_g, float az_g,
                           float gx_dps, float gy_dps, float gz_dps,
                           float dt_s, IMU_Attitude *out);

#endif /* IMU_ORIENTATION_H */
