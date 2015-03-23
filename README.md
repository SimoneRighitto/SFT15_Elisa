# SFT15 - Elisa3
----------------

##Testé et a fonctionné
- Download d'un programme dans le robot (Arduino pour le robot-side)
- Contrôle du robot via l'antenne et le programme .. (CodeBlock pour le PC-side)


## Contenu
1. [Introduction](#intro)
2. [Objectifs de ce projet](#obj)
3. [Elisa-3](#elisa)
4. [Documentation officielle](#doc)
5. [Matériel utilisé](#mat)
6. [Environnement de développement](#envi)
7. [Fonctionnalités et implémentation](#concept)
8. [Mise en place finale](#place)
9. [Conclusion](#conlusion)
10. [Liens](#liens)


*Note : En italique se trouvent les points à confirmer avec l'équipe et avec leur faisabilité.*

## <a name="intro"></a>1. Introduction

Elisa-3 est un robot conçu par [GCtronic](http://www.gctronic.com/) et équipé notamment de capteurs, d’émetteurs infrarouges et de Leds RGB qui permettent de colorer le couvercle du robot. 

Le but général de ce projet est de pouvoir mettre en place un scénario intéressant utilisant des robots. En effet, la Maison d’Ailleurs, musée de la science-fiction situé à Yverdon-les-Bains, mettra à notre disposition un espace afin de montrer notre travail, dans le cadre de sa future exposition sur la thématique des robots. 

Le projet décrit par la suite est en réalité une reprise d'un premier projet (réalisé par Marcel Sinniger, Dominique Jollien, Stéphane Maillard et Auriana Hug). Nous avons donc pu nous baser sur une première expérience. Toutefois, comme nous le verrons par la suite, nous avons fait face à de nouveaux challenges.  

En outres, ce document explique les différents outils de programmation nécessaires pour travailler avec le robot Elisa-3. Les fonctionnalités imaginées dans notre scénario seront expliquées au niveau de leur mise en œuvre et de leur implémentation. 


## <a name="obj"></a>2. Objectifs de ce projet

Pour notre projet, nous avons imaginé un concept basé sur les caractéristiques du robot Elisa-3, que nous décrirons dans le chapitre suivant. De plus, nous nous sommes inspirés de l'idée de colonie. En effet, nous aurons à disposition une dizaine de robots, et leur petite taille  permet de mettre en place aisément un environnement pour plusieurs robots de son type. Dès lors, il est intéressant de les utiliser ensemble dans cette optique. 

Ainsi, nous pourrons réutiliser une partie du code déjà conçu lors du projet précédent, et l'objectif est d'y ajouter d'autres fonctionnalités, afin de rendre un projet ludique et fonctionnel pour la Maison d'Ailleurs.


## <a name="elisa"></a>3. Elisa-3

### 3.1 Déplacement 
Pour se déplacer et tourner, le robot Elisa-3 possède deux roues indépendantes, chacune étant couplée à son propre moteur DC.  De plus, un accéléromètre sur trois axes permet au robot de connaître son orientation dans l’espace. Le déplacement du robot et la charge sont indépendants de la gravité. Le robot travaille également à la verticale et à l'envers, grâce à ses aimants. Sa position peut être exploitée par le programme du robot afin de changer le comportement du robot, par exemple. 

### 3.2 Détection d’objets 
Elisa-3 est équipé de 8 émetteurs infrarouges couplés avec des senseurs répartis sur le côté de l’appareil. Ils permettent notamment la détection d’objets à proximité, ainsi que l’évitement de chutes lors de l’approche d’un bord de table. 

### 3.3 Communication avec un ordinateur 
Le robot possède aussi une antenne RF afin de communiquer avec un ordinateur. Il envoie les informations de ses capteurs et peut recevoir des instructions. En outres, le robot est équipé d’un processeur ATMEL ATmega2560 compatible Arduino. Un port micro-USB permet la connexion à un ordinateur à l'aide d'un câble micro USB, afin de changer le code du robot. Un sélecteur à 8 positions est présent sur le PCB.  

### 3.4 Communication avec un autre robot
Ils peuvent également communiquer des informations avec un robot semblable. 

### 3.5 Couleurs 
Une led RGB centrale permet au robot d’afficher un large choix de couleurs sur le dessus du l’appareil. Elisa-3 dispose aussi de huit leds vers sur le côté.

### 3.6 Batterie 
Une batterie équipe le robot. Elle peut être chargée soit en le branchant avec un câble micro USB à l'ordinateur, soit en positionnant les deux contacts dorés sur une station de rechargement adaptée. Lors du rechargement, une petite led rouge frontale s'allume.


## <a name="doc"></a>4. Documentation officielle

Le site officiel du fournisseur décrit les différentes manières de programmer et de gérer le robot Elisa-3. Voici le lien : http://www.gctronic.com/doc/index.php/Elisa-3


## <a name="mat"></a>5. Matériel utilisé

Pour ce projet, nous avons utilisé : 

- 8 robots Elisa-3  
- 1 antenne RF (avec son câble) 
- 2 bases de recharges (avec leur câble) 
- 1 environnement (l’arène) 


## <a name="envi"></a>6. Environnement de développement

GCtronic propose plusieurs IDE pour programmer les robots. Le seul que nous avons pu utiliser est **[Arduino](http://www.arduino.cc/)**. Malheureusement, la majorité des exemples de code sur le site du fournisseur ont été conçus pour AVR Studio. En outres, la traduction du code est pénible, car la librairie n’est pas complétement la même pour les deux IDE. Ainsi, la librairie "Avancée" doit être utilisée avec AVR Studio. Il s'est avéré impossible à la faire fonctionner en utilisant Arduino

6.1 Installation du logiciel Arduino 
Cette page web du fournisseur explique comment installer Arduino : http://www.gctronic.com/doc/index.php/Elisa-3#Arduino_IDE_project  
Après l’installation et la configuration de ce logiciel, il est nécessaire de faire un premier test. Pour ce faire, il est possible de suivre les instructions situées juste après la section qui décrit les modifications du fichier « boards.txt ».  
L'utilisation des programmes développés pendant le cours nécessite l'utilisation de la librairie côté robot basique que nous avons modifié. Celle-ci a notamment dans le fichier constants.h des valeurs différentes pour LINE_IN_THR LINE_OUT_THR.  
Comme indique « Tools=>Serial Port », il faut choisir le port COM pour établir la connexion avec le robot. Nous avons rencontré des problèmes avec les ports USB 3.0. En effet, la connexion au robot ne fonctionne pas avec ces derniers. Heureusement, nous avons constaté que ceci fonctionne avec les ports USB 2.0. Sur les ordinateurs portables de LeNovo, on peut utiliser les ports USB, identifiés en jaune (ports USB 2.0).   

6.2 Développement avec l’antenne
Pour qu’on puisse communiquer depuis un ordinateur avec les robots à l’aide de l’antenne, il faut tout d’abord installer quelques outils. Les procédures d’installation et de configuration sont décrites sur la page du fournisseur d’Elisa-3 : http://www.gctronic.com/doc/index.php/Elisa-3#PC_side  
La mise en œuvre d’un projet « PC-Side » est pénible. C’est pourquoi nous avons préparé une version de CodeBlocks portable qui est déjà préconfigurée (voir le chapitre 9 « Contamination »). Il faut installer MinGW1 dans le dossier C:\MinGW. Lors de l’installation, il est nécessaire de sélectionner les compilateurs C et C++ de MinGW, afin de les installer. Lorsque la procédure est terminée, il faut brancher l’antenne sur un port USB 2.0.  En effet, le problème mentionné dans le chapitre précédent peut survenir. 

6.3 Debugging avec l'antenne
La communication à l’aide de l’antenne permet aussi de récupérer des valeurs des capteurs des robots. En effet, les robots peuvent envoyer des valeurs au PC. Ceci permet de mieux comprendre ce qui se passe sur le robot. Le projet précompilé du fournisseur dans le chapitre « PC side » montre comment des valeurs peuvent être échangées. Une version exécutable de ce projet se trouve dans le dossier « Debugging\Debugging_avec_l_antenne » De plus, le sous-chapitre « Code du côté PC » du chapitre « Contamination » exploite cette fonctionnalité pour la récupération des états des batteries des robots.


## <a name="concept"></a>7. Fonctionnalités et implémentation

**1) Mode terminé**

*Pour chaque fin de mode, tous les robots se déplacent aléatoirement et vont se recharger aléatoirement.*

**2) Rechargement**

*Dès que la batterie d'un robot est presque déchargée, le robot quitte son mode actuel et va se recharger. La couleur de ce mode doit être unique.*

**3) Contamination**

Un des robot est contaminé. Son but est de contaminer tous les autres robots. Ceux-ci sont immobiles et ont une couleur les distinguant du robot contaminé.
Dès qu'un robot sain est touché par un robot contaminé, ce dernier prend le mode "contaminé" et commense à se déplacer. A la fin du mode, les robots se mettent en mode "terminé".

**4) Contamination contrôlée**

Même chose que la Contamination, sauf qu'un visiteur peut diriger lui-même le premier robot contaminé.

**5) Parade ?**

*Groupe de 4 robots qui font les mêmes déplacements de manière synchonisée.*


## <a name="place"></a>8. Mise en place finale

###Arène
- Pour 7 robots (2 en réserve)
- Dimensions: 65 x 55 cm 
- Surface: blanche, aimantée et lisse
- Disposition: verticale
- Forme: rectangle avec des bords (barrières)



## <a name="conlusion"></a>9. Conclusion



## <a name="liens"></a>10. Liens

- [GCtronic, Elisa-3](http://www.gctronic.com/doc/index.php/Elisa-3)





