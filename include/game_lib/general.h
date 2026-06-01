/*
 * general.h
 *
 *  Created on: 31 may. 2026
 *      Author: Usuario
 */

#ifndef GENERAL_H_
#define GENERAL_H_

// Función auxiliar para leer la tecla actual basada en los GPIO
uint8_t LeerTeclaMatricial(void) {
    // Se lee el registro FIO0PIN, se enmascara y se determina qué columna está en bajo (o alto, según lógica).
    uint32_t columnas = (LPC_GPIO0->FIOPIN >> 4) & 0x0F;
    // Retorna un identificador de tecla.
    // Lógica de mapeo.
    // return mapeo_tecla;
}

#endif /* GENERAL_H_ */
