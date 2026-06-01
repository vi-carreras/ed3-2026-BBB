/*
 * general.c
 *
 *  Created on: 1 jun. 2026
 *      Author: Usuario
 */
void retardo(){//retardo del teclado
	uint32_t i;  //for(i=0;i<55000;i++){};//se purde ir reajustado conforme a la necesidad llegamos a ese valor porque se estabiliza el teclado
	for(i=0;i<80000;i++){};
}

uint8_t EscanearTeclado(void){
	for(fila=0;fila<4;fila++){//recorre las cuatro filas que son las salidas enviando "0"
		switch(fila){
			case(0): LPC_GPIO2->FIOCLR|=1<<0; break;//envio un cero al P2.0
			case(1): LPC_GPIO2->FIOCLR|=1<<1; break;//envio un cero al P2.1
			case(2): LPC_GPIO2->FIOCLR|=1<<2; break;//envio un cero al P2.2
			case(3): LPC_GPIO2->FIOCLR|=1<<3; break;//envio un cero al P2.3
			}//llave del switch
		retardo();
		while(!(LPC_GPIO2->FIOPIN>>4 &(0XF)));
		retardo();
		scan_data = LPC_GPIO2->FIOPIN>>4 &(0XF);//desplazo 4 lugares a las entradas y verifica si se ha presionado alguna tecla envia un cero porque son entradas
		columna = -1;//es un CONTROL para saber si se presiono una tecla
		switch(scan_data){
			case(0xE): columna =0;break;
			case(0xD): columna =1;break;
			case(0xB): columna =2;break;
			case(0x7): columna =3;break;
		}//llave del switch

		if(columna!=-1){//este algoritmo con break hace que salga del bucle for si es que presiona una tecla
			break;
			}
	}//==== Llave del for  fila i============================================
	//==============salida del ciclo for se evalua como sale columna = -1 o  !=-1======================================

		if(columna ==-1){// Si no cambia columna =-1 es porque no se presiono ninguna tecla no retorna nada
			return 0;
		}
		else{//si columna != -1 entonces se ha presionado una tecla
			dato_uart = keyboard[fila][columna];// dato_uart es un dato globa; listo para enviar a un uart
			enviar(dato_uart);//funcion que envia por el uart la tecla que se presiono
			return keyboard[fila][columna];//la funcion uint8_t ScanearTeclado() retorna un elememto del arreglo
		}
}
