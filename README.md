# SFT15 - Elisa3
----------------

*Note : En italique se trouvent les points à confirmer avec l'équipe et avec leur faisabilité.*

##Arène##
- Pour 7 robots (2 en réserve)
- Dimensions: 65 x 55 cm ?
- Surface: blanche, aimantée et lisse
- Disposition: verticale
- Forme: rectangle avec des bords (barrières)


##Testé et a fonctionné##
- Download d'un programme dans le robot (Arduino pour le robot-side)
- Contrôle du robot via l'antenne et le programme .. (CodeBlock pour le PC-side)


##Concepts##
**1) Contamination**

Un des robot est contaminé. Son but est de contaminer tous les autres robots. Ceux-ci sont immobiles et ont une couleur les distinguant du robot contaminé.
Dès qu'un robot sain est touché par un robot contaminé, ce dernier prend le mode "contaminé" et commense à se déplacer. A la fin du mode, les robots se mettent en mode "terminé".

**2) Contamination contrôlée**

Même chose que la Contamination, sauf qu'un visiteur peut diriger lui-même le premier robot contaminé.

**3) Parade ?**

*Groupe de 4 robots qui font les mêmes déplacements.*

###Mode terminé###

*Pour chaque fin de mode, tous les robots se déplacent aléatoirement et vont se recharger aléatoirement.*

###Batterie presque vide###

*Dès que la batterie d'un robot est presque déchargée, le robot quitte son mode actuel et va se recharger. La couleur de ce mode doit être unique.*
