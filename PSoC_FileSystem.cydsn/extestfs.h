#ifndef EXTESTFS_H
#define EXTESTFS_H

void exInit(void);
void exCreateFile(void);
void exReadFile(void);
void exFormat(void);
void exDriveInfo( void );
void exDirectory( void );
void exPrintSector( void  * driver, void * data, unsigned long sector );
#endif