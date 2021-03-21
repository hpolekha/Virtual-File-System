#!/bin/sh

VFS_FILE_NAME='drive.vfs'
make
echo '~~~~~~~~~~~~ TESTOWANIE WIRTUALNEGO SYSTEMU PLIKOW ~~~~~~~~~'
echo 'Zalozenia: '
echo  ' - jednopoziomowy system plikow'
echo  ' - dysk jest dzielony na mniejsze bloki'
echo  ' - rozmiar kazdego bloku wynosi 2046 bajtow (2 kB)'
echo  ' - w zaleznosci od rozmiaru kazdy plik dostaje jeden lub wiecej blokow'
echo  ' - mozliwosc fragmentacji'
echo  ' - alokacja z lista polonczen'
echo  '~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~'

echo 'Tworzymy dysk o rozmiarze 10240 bajtow.'
echo 'Taki dysk powinien posiadac 4 inody + 4 bloki po 2kb + superblock'
./vfs $VFS_FILE_NAME create_fs 10240
echo  ''
echo 'Stan dysku:'
./vfs $VFS_FILE_NAME show_all
echo ''
echo 'Zapisujemy plik o rozmiarze 2kb. Powinien zajac dokladnie jeden, pierwszy blok z indeksem 0'
./vfs $VFS_FILE_NAME insert 2kb.txt 2kb
echo ''
echo 'Stan dysku:'
./vfs $VFS_FILE_NAME show_all
echo ''
echo 'Zapisujemy plik o rozmiarze 2kb + 1. Powinien zajac dwa bloki z indeksamy 1 i 2'
./vfs $VFS_FILE_NAME insert 2k1b.txt 2k1b
echo ''
echo 'Stan dysku:'
./vfs $VFS_FILE_NAME show_all
echo ''
echo 'Lista plikow dysku:'
./vfs $VFS_FILE_NAME show_cat
echo ''
echo 'Probojemy zapisac plik o rozmiarze 2kb + 1: '
./vfs $VFS_FILE_NAME insert 2k1b.txt 2k1b_2
echo 'Proba skonczyla sie bledem - za malo miejsca'
echo ''
echo 'Probojemy zapisac plik o rozmiarze 2kb:'
./vfs $VFS_FILE_NAME insert 2kb.txt 2kb
echo 'Proba dodania pliku o rozmiarze jednego bloku, ale o nazwie 2kb - tez skonczy sie bledem - plik o tej nazwie juz istnieje, a domyslnie nie pozwalamy kopiowac'
echo 'Stan dysku:'
./vfs $VFS_FILE_NAME show_all ##
echo ''
echo 'Przyklad fragmentacji.'
echo '     Usuniemy plik 2kb i dodamy drugi plik o rozmiarze 2kb + 1'
./vfs $VFS_FILE_NAME remove 2kb
./vfs $VFS_FILE_NAME insert 2k1b.txt 2k1b_2
echo '     Plik zostal rozdzielony na dwa bloki.'
echo ''
echo 'Stan dysku:'
./vfs $VFS_FILE_NAME show_all

echo 'Usuwamy srodkowy plik o rozmiarze 2kb+1 oraz dodamy jeden pusty plik (o rozmiarze 0)'
./vfs $VFS_FILE_NAME remove 2k1b
./vfs $VFS_FILE_NAME insert 0b.txt 0b
echo ''
echo 'Stan dysku:'
./vfs $VFS_FILE_NAME show_all 
echo ''
echo 'Wyciagamy wszystkie pliki z systemu wirtualnego'
./vfs $VFS_FILE_NAME get 2k1b_2 2k1b_2.downloaded
./vfs $VFS_FILE_NAME get 0b 0b.downloaded
echo ''
echo 'Konczymy prace. Usuwamy dysk.'
./vfs $VFS_FILE_NAME delete

