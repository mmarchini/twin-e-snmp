# TwinEngine+SNMP [ TwinEngine ]

Esse é um projeto para a disciplina de [INF01015] Gerência e Aplicação em Redes do curso de Ciência da 
ComutaçãoUniversidade Federal do Rio Grande do Sul. 

O objetivo do projeto é extender um agente SNMP (nesse caso, o Net-SNMP) para disponibilizar uma MIB
customizada que permita acessar e alterar os dados de um jogo. Para tal, foi escolhido o projeto open-source
TwinEngine, que é uma re-implementação do jogo Little Big Adventure, desenvolvido pela Adeline Software 
International em 1994. 
Meu objetivo é possibilitar manipulação do jogo através de uma Interface Web, que se comunica com um Web-Server
que também exerce o papel de Gerente SNMP, se comunicando com o Agente responsável pela comunicação com o jogo. 

O código presente nesse repositório é um fork do projeto TwinEngine, buscando adicionar uma forma do Agente SNMP
obter e alterar os dados do jogo, através de UNIX Sockets.

Os arquivos relevantes para a implementação da comunicação entre o agente e o jogo são os seguintes:

```bash
src/sockmanager.c
src/sockmanager.h
src/lbaengine.c
```

# TwinEngine: a Little Big Adventure engine
	
Copyright (C) 2013 The TwinEngine team
Copyright (C) 2008-2013 Prequengine team
Copyright (C) 2002-2007 The TwinEngine team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License 
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


## - Brief description -

Welcome to TwinEngine project.
This project allows you to play Little Big Adventure 1 (LBA1 for short) game.

Will be release various public versions during development for testing proposes.
We appreciate everyone's help to fix all the issues the game may have.

This project was merged with prequengine.


## - Not available original features (yet) -

The game is at beta stage with most the game features implemented. The entire game is compatible
and few features are missing.  
Check the TODO list at https://code.google.com/p/twin-e/wiki/TODO for more informations


## - How to use it -

For public releases, you just need to copy all package content to LBA1 (or other projects)
game directory. Some files must be overriden to properly run the application engine.

For the compiled version, you just need to copy the application engine and files on folder trunk\bin
to LBA1 game directory to play it. You will need the original game to play with this engine.


## - Compiling -

You need libSDL, libSDL_Mixer in order to properly compile twin-e.
As addicional for in-game debug you'll need libSDL_ttf.


## on GNU/Linux:

After installing the libsdl, libsdl_mixer and libsdl_ttf development packages,
go to the trunk/src directory and type "make" (without the quotes, off course).
This will compile the source code and create an executable file called
"twin-e". Copy this file to the same place in your system where you have
made a copy of the "lba" directory from the LBA1 CDROM. Also copy the "lba.cfg"
file to that location. Go to that dir and type "./twin-e"

If you are a Ubuntu user, the procedure should look like this:

```bash
sudo apt-get install libsdl1.2-dev libsdl-mixer1.2-dev libsdl-ttf2.0-dev
cd trunk/src
make
cp ./lba.cfg ~/Desktop/lba/
cp ./twin-e ~/Desktop/lba/
cd ~/Desktop/lba
./twin-e
```

(supposing that you have made a copy of the LBA1 CDROM's "lba" directory in your Desktop)


## - License -

Refer COPYING file for full license descriptions.


## - Authors -

Refer AUTHORS file for a full list of contributions on this project.
