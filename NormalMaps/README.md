# Problema 2 - Normal Mapping Without Precomputed Tangents
## Descri��o do problema
Um plano de ch�o, dois pontos de luz, um conjunto de 5 esferas flutuando. Uma esfera vermelha ao centro.

Pelo menos uma esfera reflexiva

Pelo menos uma esfera transl�cida com material refrat�rio vidro (�ndice 1.5)

A c�mera em movimento de rota��o em rela��o ao eixo Y com uma esfera ao centro, dando a sensa��o de corpos em �rbita da esfera central
## Arquivos
```
NormalMaps
?? README.md - instru��es do programa - voc� est� este arquivo :)
?? ETAPAS.md - etapas de desenvolvimento do programa
?? objetos.h - defini��o das classes do programa
?? globals.h - vari�veis globais
?? main.cpp - programa principal
?? raycasting.h - defini��o das fun��es do raycasting
?? raycasting.cpp - fun��o recursiva para tra�ar raios e c�lculo da cor
?? Makefile.linux64 - makefile para Linux
?? Makefile.MacOS - makefile para Mac
?? makelinux.sh - script para compila��o Linux
?? makemac.sh - script para compila��o Mac
```
## Compila��o
Para Windows 10
```
./makelinux.sh
```
Para MacOS
```
./makemac.sh
```
## Utilizando o programa
Para executar o programa
```
./main
```
Aparecer� a quantidade de FPS na barra superior da janela do programa

Para ligar desligar fontes de luz

Teclar L ou l. Ser�o ligados e desligados os pontos de luz em uma sequ�ncia programada

Para selecionar uma esfera

Digitar um n�mero de 0 a 5

Para movimentar uma esfera

Utilizar uma das setas direcionais Left, Right, Up e Down

OBS: esta op��o n�o se mostrou perform�tica. Tem que dar toques cont�nuos para funcionar
