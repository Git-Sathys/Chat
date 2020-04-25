int open (char tmp_buffer[])
{
    FILE* fichier = NULL;
    fichier = fopen("info.LOG","a");
    
 
    if (fichier != NULL)
    {   
        
        fputc("Coucou", fichier); // Écriture du caractère A
        fclose(fichier);

    }
    return 0;
}

/* fopen example */
#include <stdio.h>
int main ()
{
  FILE * pFile;
  pFile = fopen ("myfile.txt","w");
  if (pFile!=NULL)
  {
    fputs ("fopen example",pFile);
    fclose (pFile);
  }
  return 0;
}