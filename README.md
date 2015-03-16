# SFT15 - Elisa3
----------------

##Arène##
- Pour 7 robots (2 en réserve)
- Dimensions:
- Surface: blanche, aimantée et lisse
- Disposition: verticale
- Forme: rectangle avec des bords (barrières)


##Testé et a fonctionné##
- Download d'un programme dans le robot
- Contrôle du robot via l'antenne et le programme .. (CodeBlock)


##Concepts##
**1) Contamination**

Un des robot est contaminé. Son but est de contaminer tous les autres robots. Ceux-ci sont immobiles et ont une couleur les distinguant du robot contaminé.
Dès qu'un robot sain est touché par un robot contaminé, ce dernier prend le mode "contaminé" et commense à se déplacer. A la fin du mode, les robots se mettent en mode "terminé".

**2) Contamination contrôlée**

Même chose que la Contamination, sauf qu'un visiteur peut diriger lui-même le premier robot contaminé.

**3) Parade ?**

...

###Mode terminé###

*Pour chaque fin de mode, tous les robots se déplacent aléatoirement et vont se recharger aléatoirement.*

###Batterie presque vide###

*Dès que la batterie d'un robot est presque déchargée, le robot quitte son mode actuel et va se recharger. La couleur de ce mode doit être unique.*
