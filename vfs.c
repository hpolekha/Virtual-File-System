/**************************************************************************
 *                      Virtual File System                                 *
 * autor: Halyna Polekha                                                  *
 **************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>						/*unlink()*/

#define FILE_SYSTEM_NAME 32				/*strcpy, strncmp*/
#define ELEMENT_SIZE 2048
#define SPACE_USED (1 << 0)				/*czy wezel jest wolny, czy zajety*/	/*flagi */
#define FIRST_FILE_ELEMENT (1 << 1)		/*czy jest poczatkiem, lub kontynuacja pliku*/

struct file_system     /*superblock*/
{
	FILE * F;							
	unsigned int number_of_nodes;		/*ilosc wezlow*/
	struct file_system_node * inodes;	/*lista wezlow*/
};

struct file_system_disk_size   /*boot block*/	/*pierwszy blok fs*/
{
	size_t size;						/*rozmiar dysku*/
};

struct file_system_node	 /*i-nodes*/				/*wezel*/
{
	unsigned int flags;
	char name[FILE_SYSTEM_NAME];		/*nazwa pliku (dowolna wartość, jeśli ~IS_START)*/
	unsigned int size;					/*rozmiar danych w bloku odpowiadającym danemu węzłowi*/
	unsigned int next;					/*indeks następnego węzła lub -1, jeśli jest to ostatni węzeł*/
};

/*Bloki danych będą zawierały wyłącznie dane, bez informacji o strukturze.*/
struct file_system * file_system_create(const char * f_name, const size_t size); /*utworz wirtualny dysk*/
struct file_system * file_system_open(const char * f_name); /*otworz dysk wirtualny*/
void file_system_close(struct file_system * v);		/*zamykanie dysku*/
void file_system_delete(const char * f_name);		/*usuwanie dysku wirtualnego*/
void file_system_dump(const struct file_system * v); /*Wyświetlanie zestawienia z aktualną mapą zajętości dysku (?)*/
void file_system_list(const struct file_system * v); /*wyswietlanie katalogu dysku file_system*/
int file_system_copy_to(const struct file_system * v, const char * source_f_name, const char * destination_f_name); /* kopiowanie z minix na file_system*/
int file_system_copy_from(const struct file_system * v, const char * source_f_name, const char * destination_f_name); /*kopiowanie z minix na file_system*/
int file_system_delete_file(const struct file_system * v, const char * f_name); /*usuwanie pliku z file_system*/
unsigned int file_system_required_inodes_for(const size_t size);



/*===========================================================================*
 *                       file_system_create                                  *
 *===========================================================================*/

struct file_system * file_system_create(const char * f_name, const size_t size) 
/**/
{
	FILE * F;

	char zero_block[128];							/**/
	memset(zero_block, 0, sizeof(zero_block));

	unsigned int number_of_nodes;					/*M wezlow*/

	size_t free_space = size;						/*wolnych bajtow*/
	size_t bytes_to_write;

	struct file_system_disk_size ds;				/*inicjuje pierwszy blok*/
	ds.size = size;
	struct file_system_node * inodes;				/*wsk na liste wezlow*/
	struct file_system * v;							/*wsk na fs*/

	F = fopen(f_name, "wb");						/*stworz plik o zadanej nazwie i rozmiarze*/
	if (!F)
		return 0/*NULL*/;
									/*Wypelniamy "zerami"*/
	while (free_space > 0)							/*piszemy do pliku dopoki mamy miejsce*/
	{
		bytes_to_write = sizeof(zero_block);		/*piszemy po 128 bajtow*/
		if (bytes_to_write > free_space)			/*jesli zostalo < niz 128 bajtow, to piszemy tyle, ile zostalo*/
			bytes_to_write = free_space;

		fwrite(zero_block, 1, bytes_to_write, F);

		free_space -= bytes_to_write;
	}

	fseek(F, 0, SEEK_SET);							/*ustawiamy pozycje indykatora na poczatek*/

	fwrite(&ds, sizeof(ds), 1, F);					/*piszemy disksize*/

	number_of_nodes = (size - sizeof(struct file_system_disk_size)) / (sizeof(struct file_system_node) + ELEMENT_SIZE); /*policz ilosc blokow*/

	inodes = malloc(sizeof(struct file_system_node) * number_of_nodes); /*przydziel pamiec na wezle*/
	
	unsigned int i;
	for (i = 0; i < number_of_nodes; i++)			/*ustaw flagi*/
		inodes[i].flags = 0;

	v = malloc(sizeof(struct file_system));			/*przydziel pamiec na file_system*/
	v->F = F;
	v->number_of_nodes = number_of_nodes;
	v->inodes = inodes;								/*wsk na wezle*/

	return v;
}
/*===========================================================================*
 *                       file_system_open                                    *
 *===========================================================================*/
struct file_system * file_system_open(const char * f_name)
{
	FILE * F;

	size_t size_file;
	unsigned int number_of_nodes;

	struct file_system_disk_size ds;
	struct file_system_node * inodes;
	struct file_system * v;

	F = fopen(f_name, "r+b");						/*otworz plik o zadanej nazwie*/
	if (!F)
		return NULL;

	fseek(F, 0, SEEK_END);
	size_file = ftell(F);      

	if (size_file < sizeof(struct file_system_disk_size))
	{
		fclose(F);
		return 0/*NULL*/;
	}
	fseek(F, 0, SEEK_SET);

	/*nalezy odpowiednio zachowac sie dla pliku o rozmiarze 0 bajtow*/
	if (fread(&ds, sizeof(ds), 1, F) <= 0)			/*odczytaj rozmiar pliku do ds*/			/*Jesli rozmiar pliku == 0*/
	{
		fclose(F);
		return 0/*NULL*/;
	}

	if (ds.size != size_file)						/*porownaj rozmiar ds z rozmiarem pliku (sprawdzenie bledow)*/
	{
		fclose(F);
		return 0/*NULL*/;
	}

	number_of_nodes = (size_file - sizeof(struct file_system_disk_size)) / (sizeof(struct file_system_node) + ELEMENT_SIZE); /* oblicz ilosc blokow */

	inodes = malloc(sizeof(struct file_system_node) * number_of_nodes);
	if (fread(inodes, sizeof(struct file_system_node), number_of_nodes, F) <= 0) /*skopiuj wezly do pam operacyjnej*/
	{
		fclose(F);
		free(inodes);
		return 0/*NULL*/;
	}

	v = malloc(sizeof(struct file_system));
	v->F = F;
	v->number_of_nodes = number_of_nodes;
	v->inodes = inodes;

	return v;
}
/*===========================================================================*
 *                       file_system_close                                   *
 *===========================================================================*/
void file_system_close(struct file_system * v)		/*zamykanie dysku*/
{
	fseek(v->F, sizeof(struct file_system_disk_size), SEEK_SET);
	fwrite(v->inodes, sizeof(struct file_system_node), v->number_of_nodes, v->F); /*zapisz wezly na dysku*/

	fclose(v->F);	/*zamknij plik*/
	free(v->inodes);
	free(v);
	v = NULL;
}
/*===========================================================================*
 *                       file_system_delete                                  *
 *===========================================================================*/
void file_system_delete(const char * f_name)		/*usuwanie dysku wirtualnego*/
{
	unlink(f_name);									/*usun plik o zadanej nazwie (remove a link to a file)*/
}
/*===========================================================================*
 *                        file_system_dump                                   *
 *===========================================================================*/
void file_system_dump(const struct file_system * v) /*Wyświetlanie zestawienia z aktualną mapą zajętości dysku (?)*/
{
	unsigned int i;
	unsigned int free_nodes = 0;

	printf("Inodes:\n");
	for (i = 0; i < v->number_of_nodes; i++)
	{
		printf(" Nr: %d\n", i);
		printf(" Flags: %d\n", v->inodes[i].flags);
		printf(" Name: %s\n", v->inodes[i].name);
		printf(" Size: %d\n", v->inodes[i].size);
		printf(" Next: %d\n", v->inodes[i].next);
		printf("\n");
	}

	printf("Space:\n"); /*Zliczaj węzły z flagą IN_USE i podziel przez całkowitą liczbę węzłów aby uzyskać informację o zajętości*/
	for (i = 0; i < v->number_of_nodes; i++)
	{
		if (!(v->inodes[i].flags & SPACE_USED))
			free_nodes++;

		printf("%c", (v->inodes[i].flags & SPACE_USED ? (v->inodes[i].flags & FIRST_FILE_ELEMENT ? '*' : '+') : '-'));	/*---free--(*++++file+++++++)(*+++file++)------- */
	}

	printf("\n");
	printf("Free inodes: %d/%d\n", free_nodes, v->number_of_nodes);
}

/*===========================================================================*
 *                          file_system_list                                 *
 *===========================================================================*/
void file_system_list(const struct file_system * v) /*wyswietlanie katalogu dysku file_system*/
{
	unsigned int i;
	for (i = 0; i < v->number_of_nodes; i++) /*Przechodząc całą listę węzłów wypisuj te z flagami IN_USE | IS_START*/
	{
		if ((v->inodes[i].flags & SPACE_USED) && (v->inodes[i].flags & FIRST_FILE_ELEMENT))
		{
			printf("File: %s @%d\n", v->inodes[i].name, i);
		}
	}
}

/*===========================================================================*
 *                        file_system_copy_to                                *
 *===========================================================================*/
int file_system_copy_to(const struct file_system * v, const char * source_f_name, const char * destination_f_name) /* kopiowanie z minix na file_system*/
{
	FILE * source_file;
	size_t source_file_length;
	unsigned int required_inodes; 
	unsigned int * nodes_queue;
	unsigned int current_inode;
	unsigned int current_queue_inode;
	unsigned int i;
	char read_buffer[ELEMENT_SIZE];

	if (strlen(destination_f_name) == 0) /*jak dlugosc nazwy wyjsciowego 0 */
		return -1;

	for (i = 0; i < v->number_of_nodes; i++)
	{
		if (v->inodes[i].flags & SPACE_USED && v->inodes[i].flags & FIRST_FILE_ELEMENT && strncmp(v->inodes[i].name, destination_f_name, FILE_SYSTEM_NAME) == 0)
			return -2; /*jesli juz jest plik o takiej nazwie*/
	}

	source_file = fopen(source_f_name, "rb");
	if (source_file == 0/*NULL*/)
		return -3;

	fseek(source_file, 0, SEEK_END); /*kursor na koniec pliku*/
	source_file_length = ftell(source_file); /*oblicz rozmiar pliku*/
	fseek(source_file, 0, SEEK_SET); /*begin*/

	required_inodes = file_system_required_inodes_for(source_file_length); /*policz ile potrzebujemy blokow*/
	nodes_queue = malloc(required_inodes * sizeof(unsigned int)); /*przydzielamy potrzebna pamiec operacyjna dla kolejki blokow*/

	/* Sprawdź, przechodząc po wszystkich węzłach, czy w systemie jest wystarczająco */
	/*dużo miejsca (oblicz ilość węzłów bez flagi IN_USE) - zapamiętuj numery kolejnych */
	/*pustych węzłów aż sumaryczny rozmiar odpowiadających im bloków danych */
	/*przekroczy rozmiar pliku.*/

	current_queue_inode = 0;
	for (current_inode = 0; current_inode < v->number_of_nodes; current_inode++)
	{
		if ((v->inodes[current_inode].flags & SPACE_USED) == 0)
			nodes_queue[current_queue_inode++] = current_inode; /*do kolejki przepisujemy kolejne indeksy wolnycj blokow*/
		if (current_queue_inode == required_inodes) /*Jesli juz mamy potrzebna ilosc*/
			break;
	}

	/* Jeśli nie przekroczy – wyświetl błąd. Należy */
	/*odpowiednio zachować się dla pliku o rozmiarze 0. */

	if (current_queue_inode < required_inodes)
	{
		free(nodes_queue);
		fclose(source_file);

		return -4;
	}

	/*Zapisuj informacje do węzłów (w pierwszy nazwa, w pozostałych nie jest konieczna) a dane do bloków */

	for (i = 0; i < required_inodes; i++)
	{
		v->inodes[nodes_queue[i]].flags = SPACE_USED; /*zmien flage na USED */
		v->inodes[nodes_queue[i]].size = fread(read_buffer, 1, sizeof(read_buffer), source_file); /*odczytaj 2048 bajtow z source pliku do bufora*/   /*zwroc liczbe odczyt symboli*/

		fseek(v->F, sizeof(struct file_system_disk_size) + sizeof(struct file_system_node) * v->number_of_nodes + ELEMENT_SIZE * nodes_queue[i], SEEK_SET); /*ustaw kursor gdzies???????*/

		fwrite(read_buffer, 1, v->inodes[i].size, v->F);	/*zapisz z bufora do pliku*/
		if (i == 0) /*ustaw pocatek*/
		{
			v->inodes[nodes_queue[i]].flags |= FIRST_FILE_ELEMENT;
			strncpy(v->inodes[nodes_queue[i]].name, destination_f_name, FILE_SYSTEM_NAME);
		}
		if (i < required_inodes - 1)
			v->inodes[nodes_queue[i]].next = nodes_queue[i + 1];
		else
			v->inodes[nodes_queue[i]].next = -1; /*jesli byl to ostatni el-t pliku*/
	}

	free(nodes_queue);
	fclose(source_file);

	return required_inodes;
}
/*===========================================================================*
 *                        file_system_copy_from                              *
 *===========================================================================*/
int file_system_copy_from(const struct file_system * v, const char * source_f_name, const char * destination_f_name) /*kopiowanie z file_system na minix*/
{
	FILE * destination_file;
	unsigned int i;
	unsigned int start_node;

	char buffer[ELEMENT_SIZE];

	destination_file = fopen(destination_f_name, "wb");
	if (!destination_file) /*sprawdz czy plik o pod nazwie istnieje*/
		return -1;


	start_node = -1;
	for (i = 0; i < v->number_of_nodes; i++) /*znajdz numer wezla poczatkowego*/
	{
		if (v->inodes[i].flags & SPACE_USED && v->inodes[i].flags & FIRST_FILE_ELEMENT && strncmp(v->inodes[i].name, source_f_name, FILE_SYSTEM_NAME) == 0)
		{
			start_node = i;
			break;
		}
	}

	if (start_node == -1) /* jezeli nie znaleziono pliku o takiej nazwie*/
		return -2;

	while (start_node != -1) /*Przechodząc po kolejnych węzłach odczytuj jego dane i zapisuj do pliku docelowego /dopoki nie bedzie ostatni blok*/
	{
		if (fread(buffer, 1, v->inodes[start_node].size, v->F) != v->inodes[start_node].size) /*jesli liczba odczyt != size aktualnego bloku?????*/
		{
			fclose(destination_file);
			return -3;
		}
		fwrite(buffer, 1, v->inodes[start_node].size, destination_file); 

		start_node = v->inodes[start_node].next;
	}

	fclose(destination_file);

	return 1;

}
/*===========================================================================*
 *                        file_system_delete_file                            *
 *===========================================================================*/
int file_system_delete_file(const struct file_system * v, const char * f_name)
 /*Przechodzimy po wezlach, szukamy "glowe" naszego pliku. Zaczynajac od "glowy" kasujemy kazdej czesci flage IN_USE */
{
	unsigned int i;
	unsigned int start_node;

	start_node = -1;
	for (i = 0; i < v->number_of_nodes; i++) /*znajdz poczatkowy wezel*/
	{
		if (v->inodes[i].flags & SPACE_USED && v->inodes[i].flags & FIRST_FILE_ELEMENT && strncmp(v->inodes[i].name, f_name, FILE_SYSTEM_NAME) == 0)
		{
			start_node = i;
			break;
		}
	}

	if (start_node == -1) /*nie znaleziono pliku*/
		return -1;

	while (start_node != -1) /* przechodzac po kolejnych wezlach, kasuj im flage IN_USE*/
	{
		v->inodes[start_node].flags &= ~SPACE_USED;		
		start_node = v->inodes[start_node].next; 
	}

	return 1;
}
/*===========================================================================*
 *                        file_system_required_inodes_for                    *
 *===========================================================================*/
unsigned int file_system_required_inodes_for(const size_t size)
/* Obliczamy ile inodow sie zmiesci w size*/
{
	size_t size_remaining;
	unsigned int required_inodes = 0;
	size_t current_block_length;

	size_remaining = size;

	do
	{
		required_inodes++;

		current_block_length = size_remaining;
		if (current_block_length > ELEMENT_SIZE)
			current_block_length = ELEMENT_SIZE;

		size_remaining -= current_block_length;
	} while (size_remaining > 0);

	return required_inodes;
}
/*****************************************************************************
 *                                 main                                      *
 *****************************************************************************/
int main(int ArgC, char ** ArgV)
{
	struct file_system * fs;
	char * fs_name;
	char * option;

	if (ArgC < 3)
	{
		printf("%s <fs_name> <option>\n", ArgV[0]);
		printf(
			"Options: \n"
			"- create_fs <size>\n"
			"- show_all\n"
			"- show_cat\n"
			"- insert <src_fname> <dest_fname>\n"
			"- get <src_fname> <dest_fname>\n"
			"- remove <fname>\n"
			"- delete\n"
			);
		return 1;
	}

	fs_name = ArgV[1];
	option = ArgV[2];

	if (strcmp("create_fs", option) == 0)
	{
		if (ArgC == 4)
		{
			size_t size = atoi(ArgV[3]);
			fs = file_system_create(fs_name, size);
			if (!fs)
			{
				printf("Cannot open FS\n");
				return 2;
			}
			file_system_close(fs);
		}
		else
			printf("%s <fs_name> create_fs <size>\n", ArgV[0]);
	}
	else if (strcmp("show_all", option) == 0)
	{
		if (ArgC == 3)
		{
			fs = file_system_open(fs_name);
			if (!fs)
			{
				printf("Cannot open FS\n");
				return 2;
			}

			file_system_dump(fs);

			file_system_close(fs);
		}
		else
			printf("%s <fs_name> show_all\n", ArgV[0]);
	}
	else if (strcmp("show_cat", option) == 0)
	{
		if (ArgC == 3)
		{
			fs = file_system_open(fs_name);
			if (!fs)
			{
				printf("Cannot open FS\n");
				return 2;
			}

			file_system_list(fs);

			file_system_close(fs);
		}
		else
			printf("%s <fs_name> show_cat\n", ArgV[0]);
	}
	else if (strcmp("insert", option) == 0)
	{
		if (ArgC == 5)
		{
			fs = file_system_open(fs_name);
			if (!fs)
			{
				printf("Cannot open FS\n");
				return 2;
			}

			printf("Copying file, result: %d\n", file_system_copy_to(fs, ArgV[3], ArgV[4]));

			file_system_close(fs);
		}
		else
			printf("%s <fs_name> insert <src_fname> <dest_fname>\n", ArgV[0]);
	}
	else if (strcmp("get", option) == 0)
	{
		if (ArgC == 5)
		{
			fs = file_system_open(fs_name);
			if (!fs)
			{
				printf("Cannot open FS\n");
				return 2;
			}

			printf("Getting file, result: %d\n", file_system_copy_from(fs, ArgV[3], ArgV[4]));

			file_system_close(fs);
		}
		else
			printf("%s <fs_name> get <src_fname> <dest_fname>\n", ArgV[0]);
	}
	else if (strcmp("remove", option) == 0)
	{
		if (ArgC == 4)
		{
			fs = file_system_open(fs_name);
			if (!fs)
			{
				printf("Cannot open FS\n");
				return 2;
			}

			file_system_delete_file(fs, ArgV[3]);

			file_system_close(fs);
		}
		else
			printf("%s <fs_name> remove <fname>\n", ArgV[0]);
	}
	else if (strcmp("delete", option) == 0)
	{
		if (ArgC == 3)
		{
			file_system_delete(fs_name);
		}
		else
			printf("%s <fs_name> delete\n", ArgV[0]);
	}
	else
	{
		printf("%s invalid option `%s`\n", ArgV[0], option);
		return 1;
	}


	return 0;
}
