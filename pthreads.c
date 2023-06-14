/*
 copie os .lib e .a para: c:\Arquivos de Programas (x86)\Codeblocks\mingw\lib\
 copie os .dll para: c:\Arquivos de Programas (x86)\Codeblocks\mingw\bin\
 copie os .h para: c:\Arquivos de Programas (x86)\Codeblocks\mingw\include\
 project -> build options -> linker settings -> Other linker options: inclua -lpthreadGC1
*/
#define MWIN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#ifdef MWIN
#include <windows.h>
#endif
#define ERRO 0
#define OK   1

#define  FALSE 0
#define  TRUE  1
#define BUFFERVAZIO  0
#define BUFFERCHEIO  1


#define TEMPO_SLEEP 10


/// declaracao dos mutexes
pthread_mutex_t finalizou;  // independente para a variavel "G_terminou"
pthread_mutex_t controle_buffer_entrada;  // segundo - nao usado com buffer_saida
pthread_mutex_t controle_buffer_saida;  // segundo - nao usado com buffer_entrada


/// controladores dos buffers
int estado_buffer_entrada;
int estado_buffer_saida;


/// variavel de controle do encerramento
int G_terminou;


/// buffers de processamento
char buffer_entrada[6];
char buffer_saida[6];


void m_usleep(unsigned long pause)
{
#ifdef MWIN
   Sleep(pause);
#else
   usleep(pause*1000l);
#endif

   return;
}

void init_lock(pthread_mutex_t *lock) // inicializa as variáveis de lock, fazer isto antes do inicio das threads
{
   pthread_mutex_init(lock,NULL);
   return;
}

void fini_lock(pthread_mutex_t *lock) // finalize as variaveis de lock, apos o pthread_kill
{
   pthread_mutex_destroy(lock);
   return;
}

int gerar_entrada()  // Escreve no arquivo de 00001 a 01000
{
    FILE *arq;
    int i;
    if ((arq = fopen("e.txt","wt"))==NULL)
    {
        printf("\nERRO: criando o arquivo de entrada (e.txt)\n");
        return(ERRO);
    }

    for (i = 1 ; i <= 1000; ++i)
    {
        fprintf(arq,"%05d\n",i);
    }

    fflush(arq);
    fclose(arq);

    return(OK);
}

void *leitura()
{
    int i      = 0;
    int acabou = 0;
    int aguardar;

    FILE *arq;
    printf("\nEntrei leitura");
    arq = fopen("e.txt","rt");

    if (arq == NULL)
    {
        return EXIT_FAILURE;
    }

    while (1)
    {
        if (!acabou)
        {
            do
	        {
                pthread_mutex_lock(&controle_buffer_entrada);
                aguardar = FALSE;
                if (estado_buffer_entrada == BUFFERCHEIO)
                {
                    aguardar = TRUE;
                    pthread_mutex_unlock(&controle_buffer_entrada);
                }
            } while (aguardar == TRUE);

            //inserir item no buffer de entrada
            if(fgets(buffer_entrada, sizeof buffer_entrada, arq))
            {
                estado_buffer_entrada = BUFFERCHEIO;
                pthread_mutex_unlock(&controle_buffer_entrada);
                fflush(arq);
            }else{
                pthread_mutex_unlock(&controle_buffer_entrada);
                pthread_mutex_lock(&finalizou);
                G_terminou = 2;
                // limpeza do buffer de entrada
                strcpy (buffer_entrada, "00000");
                acabou = TRUE;
                pthread_mutex_unlock(&finalizou);
            }

        }

        m_usleep(TEMPO_SLEEP);
    }
    return(NULL);
}

void *processamento()
{
    int acabou = 0;
    int i;
    int aguardar;
    char aux[6];

    memset((void *) aux,0,sizeof(char)*6);

    printf("\nEntrei processamento");

    while (1)
    {
        if (!acabou)
        {

            do
            {
                pthread_mutex_lock(&controle_buffer_entrada);
                aguardar = FALSE;
                if (estado_buffer_entrada == BUFFERVAZIO)
                {
                    aguardar = TRUE;
                    pthread_mutex_unlock(&controle_buffer_entrada);

                    pthread_mutex_lock(&finalizou);
                    if (G_terminou == 2)
                    {
                        aguardar = FALSE;
                        G_terminou = 1;
                        acabou = TRUE;
                    }
                    pthread_mutex_unlock(&finalizou);
                }

	        } while (aguardar == TRUE);

            ///copiando a string invertida da entrada para o auxiliar
            for (i = 0; i < 5; i++)
            {
                aux[i] = buffer_entrada[5 - i - 1];
            }
            aux[5] = 0;

            ///FAZER A LIMPEZA DO BUFFER DE ENTRADA
            strcpy (buffer_entrada, "00000");

            estado_buffer_entrada = BUFFERVAZIO;
            pthread_mutex_unlock(&controle_buffer_entrada);


            ///PARTE DA SAÍDA
            do
	        {
                pthread_mutex_lock(&controle_buffer_saida);
                aguardar = FALSE;
                if (estado_buffer_saida == BUFFERCHEIO)
                {
                    aguardar = TRUE;
                    pthread_mutex_unlock(&controle_buffer_saida);
                }
            } while (aguardar == TRUE);

            ///inserir item do auxiliar no buffer de saida
            strcpy (buffer_saida, aux);
            estado_buffer_saida = BUFFERCHEIO;
            pthread_mutex_unlock(&controle_buffer_saida);

        }
        m_usleep(TEMPO_SLEEP);
    }
    return(NULL);
}

void *escrita()
{
    int acabou = 0;
    int aguardar;
    FILE *arq;

    printf("\nEntrei escrita\n");
    arq = fopen("s.txt","wt");

    if (arq == NULL)
    {
        return EXIT_FAILURE;
    }

    while (1)
    {
        if (!acabou)
        {

            do
            {
                pthread_mutex_lock(&controle_buffer_saida);
                aguardar = FALSE;
                if (estado_buffer_saida == BUFFERVAZIO)
                {
                    aguardar = TRUE;
                    pthread_mutex_unlock(&controle_buffer_saida);

                    pthread_mutex_lock(&finalizou);
                    if (G_terminou == 1)
                    {
                        aguardar = FALSE;
                        G_terminou = 0;
                        acabou = TRUE;
                    }
                    pthread_mutex_unlock(&finalizou);
                }

	        } while (aguardar == TRUE);

            ///Do buffer de saída para o arquivo
            fprintf(arq, "%s\n", buffer_saida);

            ///FAZER A LIMPEZA DO BUFFER DE SAIDA
            strcpy (buffer_saida, "00000");

            estado_buffer_saida = BUFFERVAZIO;
            pthread_mutex_unlock(&controle_buffer_saida);

        }
        m_usleep(TEMPO_SLEEP);
   }

    fclose(arq);
    return(NULL);
}

void finalizar()
{
    int nao_acabou = 1;
    while (nao_acabou)
    {
        m_usleep(50);
        pthread_mutex_lock(&finalizou);
        if (G_terminou == 0)
        {
            printf("\nEm finalizar... Acabou mesmo!");
            nao_acabou = 0;
        }
        pthread_mutex_unlock(&finalizou);
    }
    return;
}

int main(void)
{
    /// declaração das pthreads
    pthread_t faz_leitura;
    pthread_t faz_processamento;
    pthread_t faz_escrita;

    /// inicializacao do G_terminou
    G_terminou = 3;

    /// inicializacao dos mutexes de lock
    init_lock(finalizou);
    init_lock(controle_buffer_entrada);
    init_lock(controle_buffer_saida);

    /// limpeza dos buffers
    strcpy (buffer_entrada, "00000");
    strcpy (buffer_saida, "00000");

    /// inicializacao dos controladores dos buffers
    estado_buffer_entrada = BUFFERVAZIO;
    estado_buffer_saida = BUFFERVAZIO;

    /// geracao do arquivo de entrada
    //gerar_entrada();
    if (!gerar_entrada())
    {
        printf("\nVou sair");
        return(1);
    }

    /// chamada das pthreads
    if (pthread_create(&faz_leitura,NULL,leitura,NULL))
    {
        printf("\nERRO: criando thread leitura.\n");
        return(0);
    }

    if (pthread_create(&faz_processamento,NULL,processamento,NULL))
    {
        printf("\nERRO: criando thread processamento.\n");
        return(0);
    }

    if (pthread_create(&faz_escrita,NULL,escrita,NULL))
    {
        printf("\nERRO: criando thread escrita.\n");
        return(0);
    }

    /// Aguarda finalizar
    finalizar();

    /// matar as pthreads
    pthread_kill(faz_leitura, 0);
    pthread_kill(faz_processamento, 0);
    pthread_kill(faz_escrita, 0);

    /// finalização dos mutexes
    fini_lock(finalizou);
    fini_lock(controle_buffer_entrada);
    fini_lock(controle_buffer_saida);

    return(0);
}
