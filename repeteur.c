
/*
 * Fichier 'repeteur.c' utilise pour le TD 5 de programmation IP.
 * Le programme 'repeteur' est un programme demon "repetant" tout
 * ce qu'on lui dit.  Il tourne sur le port 6666 et reste à l'ecoute
 * d'une eventuelle connexion.
 *
 * Historique
 *    1999/10/15 : dntt : creation
 */

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <syslog.h>

#include <signal.h>

#define MAXLEN  1024
#define PORT_REPETEUR 6666
#define NB_CONN_MAX 5

void demon () ;

void repeter (int socket_cliente) ;
int lecture_socket (int socket_cliente, char *buf) ;


/************************************************************************
 *
 * Fonction principale : 
 * On cree le demon en dupliquant le processus afin de detacher le
 * programme du processus pere
 *
 ************************************************************************/
int main ()
{
    switch (fork ())
    {
        case -1 :
            perror ("*** erreur : fork impossible : demon non cree") ;
            exit (1) ;

        case 0 :
	    /*
	     * Ce processus fils est celui qui deviendra le demon.
	     * Ingredients pour en faire un vrai et bon demons :
	     */
	    /* 1 - On cree une nouvelle session. Pas de terminal de controle */
            setsid () ;

	    /* 2 - le repertoire courant est change a la racine afin de ne pas
	     *     causer de probleme lors de demontage eventuel de partition 
	     */
	    chdir ("/") ;

	    /* 3 - Le masque sur les droits lors de creation de fichiers */
	    umask (0) ;

	    /* 4 - Comme c'est un demon qui est lance et qui est "detache" du
	     *     term, l'entree, la sortie et l'erreur standard n'ont pas de
	     *     terminal ou s'afficher. On peut donc les fermer. 
	     */
            close (0) ;
	    close (1) ;
	    close (2) ;


	    /* 6 - On lance le demon proprement dit */
            demon () ;

	    /* 7 - Si le demon s'arrete, alors on sort du processus fils. Le
	     *     programme est alors termine. */
            exit (1) ;

        default :
	    /* Le processus pere est tue. On rend la main au shell appelant. */
            exit (0) ;
    }
    return 0 ;
}

/************************************************************************
 *
 * Le programme 'demon'
 * 
 * Structure 
 *           sockaddr_in : /usr/include/netinet/in.h
 * 
 *
 *
 ************************************************************************/
void demon ()
{
    int socket_ecoute ;
    int socket_cliente ;
    int valeur_retour ;
    int salong ;
    int opt = 1 ;
    struct sockaddr_in monadr, sonadr ;
    int status ;


    /*
     * Creation de la socket d'ecoute
     */
    socket_ecoute = socket (PF_INET, SOCK_STREAM, 0) ;

    /*
     * Modification des options associees a la socket d'ecoute.
     */
    setsockopt (socket_ecoute, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, \
		sizeof (opt)) ;

    /*
     * Initialisation des parametres internet de la socket d'ecoute.
     */
    bzero ((char *) &monadr, sizeof monadr) ;
    monadr.sin_family = AF_INET ;
    monadr.sin_port = htons (PORT_REPETEUR) ;
    monadr.sin_addr.s_addr = INADDR_ANY ;
    valeur_retour = bind (socket_ecoute, (struct sockaddr *) &monadr, \
			  (int) sizeof monadr) ;

    /*
     * Socket mise en position d'ecoute ('listen'). On specifie aussi le 
     * nombre de connexions simultanees acceptees.
     */
    valeur_retour = listen (socket_ecoute, NB_CONN_MAX) ;

    /**
     * Le demon tourne indefiniment a l'ecoute d'eventuelles connections
     * 'accept' 
     * Lorsqu'un client se connecte, une socket est cree (socket_cliente).
     * on duplique le processus et le processus fils prend en charge
     * cette socket par la fonction 'repeter'.
     */
    while (1)
    {
        salong = sizeof sonadr ;

        socket_cliente = accept (socket_ecoute, (struct sockaddr *) &sonadr, \
                                 (socklen_t *) &salong) ;
 
	switch (fork ())
        {
	    case -1 : 
		break ;

	    case 0 :
                repeter (socket_cliente) ;
                close (socket_cliente) ;
		kill (getpid(),SIGTERM);

	    default :
		waitpid (-1, &status, WNOHANG);
                close (socket_cliente) ;
        }
    }
}

/************************************************************************
 *
 * La fonction qui repete
 * 
 *
 *
 ************************************************************************/
void repeter (int socket_cliente)
{ 
    int nb_lu ;
    char buf [MAXLEN] ;

    while ((nb_lu = lecture_socket (socket_cliente, buf)) > 0)
    {
	/* Si probleme de lecture sur la socket (fermee brutalement 
	 * par exemple) */
	if (nb_lu == -1 || nb_lu == 0)
	{
	    return ;
	}

	/* On repete ce qui a ete dit et on renvoie au client */
	write (socket_cliente, buf, nb_lu) ;
    }
}

int lecture_socket (int socket_cliente, char *buf) 
{
    int nb_lu  ;

    nb_lu = read (socket_cliente, buf, MAXLEN)  ;
    return (nb_lu) ;
}
