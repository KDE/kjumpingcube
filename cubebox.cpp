/* ****************************************************************************
  This file is part of the game 'KJumpingCube'

  Copyright (C) 1998-2000 by Matthias Kiefer
                            <matthias.kiefer@gmx.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

**************************************************************************** */
#include <assert.h>
#include <math.h>
#include "cubebox.h"
#include "kcubeboxwidget.h"


CubeBox::CubeBox(const int d)
    :CubeBoxBase<Cube>(d)
{
   initCubes();
}

CubeBox::CubeBox(const CubeBox& box)
      :CubeBoxBase<Cube>(box.dim())
{
   initCubes();

   int i,j;
   for(i=0;i<dim();i++)
      for(j=0;j<dim();j++)
      {
         *cubes[i][j]=*box.cubes[i][j];
      }

   currentPlayer=box.currentPlayer;
}

CubeBox::CubeBox(KCubeBoxWidget& box)
      :CubeBoxBase<Cube>(box.dim())
{
   initCubes();

   int i,j;
   for(i=0;i<dim();i++)
      for(j=0;j<dim();j++)
      {
        *cubes[i][j]=*box[i][j];
      }

   currentPlayer=(CubeBox::Player)box.player();

}


CubeBox::~CubeBox()
{
}


CubeBox& CubeBox::operator=(const CubeBox& box)
{
   if(this!=&box)
   {
      if(dim()!=box.dim())
      {
         setDim(box.dim());
      }


      for(int i=0;i<dim();i++)
         for(int j=0;j<dim();j++)
         {
            *cubes[i][j]=*box.cubes[i][j];
         }
   }

   currentPlayer=box.currentPlayer;

   return *this;
}


CubeBox& CubeBox::operator=(KCubeBoxWidget& box)
{
   if(dim()!=box.dim())
   {
      setDim(box.dim());
   }

   for(int i=0;i<dim();i++)
      for(int j=0;j<dim();j++)
      {
         *cubes[i][j]=*box[i][j];
      }

   currentPlayer=(CubeBox::Player)box.player();

   return *this;
}




bool CubeBox::simulateMove(Player fromWhom,int row, int column)
{
   bool finished;
   bool playerWon=false;

   if(cubes[row][column]->owner()!=(Cube::Owner)fromWhom && cubes[row][column]->owner()!=Cube::Nobody)
      return false;

   cubes[row][column]->increase((Cube::Owner)fromWhom);

   do
   {
      int i,j;
      finished=true;
      playerWon=true;

      // check all Cubes
      for(i=0;i<dim();i++)
      {
	      for(j=0;j<dim();j++)
         {
	         if(cubes[i][j]->overMax())
	         {
	            increaseNeighbours(fromWhom,i,j);
	            cubes[i][j]->decrease();
	            finished=false;
	         }

	         if(cubes[i][j]->owner()!=(Cube::Owner)fromWhom)
	         playerWon=false;
         }
      }

      if(playerWon)
	      return true;
   }
   while(!finished);


   return true;
}

double CubeBox::assessField(Player player) const
{
   int cubesOne=0;
   int cubesTwo=0;
   int pointsOne=0;
   int pointsTwo=0;
   Player otherPlayer = ((player==One)? Two : One);
   bool playerWon=true;
   bool otherPlayerWon=true;

   int i,j;

   for(i=0;i<dim();i++)
   {
      for(j=0;j<dim();j++)
      {
	      if(cubes[i][j]->owner()==(Cube::Owner)One)
	      {
	         cubesOne++;
	         pointsOne+=(int)pow((float)cubes[i][j]->value(),2);
	      }
	      else if(cubes[i][j]->owner()==(Cube::Owner)Two)
	      {
	         cubesTwo++;
	         pointsTwo+=(int)pow((float)cubes[i][j]->value(),2);
	      }

	      if(cubes[i][j]->owner()!=(Cube::Owner)player)
	         playerWon=false;

	      if(cubes[i][j]->owner()!=(Cube::Owner)otherPlayer)
	         otherPlayerWon=false;
      }

   }



   if(player==One)
   {
      return (int)pow((float)cubesOne,2)+pointsOne-(int)pow(cubesTwo,2)-pointsTwo;
   }
   else
      return (int)pow((float)cubesTwo,2)+pointsTwo-(int)pow(cubesOne,2)-pointsOne;

}

bool CubeBox::playerWon(Player who) const
{
   int i,j;

   for(i=0;i<dim();i++)
      for(j=0;j<dim();j++)
      {
         if(cubes[i][j]->owner()!=(Cube::Owner)who)
	         return false;
      }

   return true;
}


void CubeBox::increaseNeighbours(CubeBox::Player forWhom,int row,int column)
{
   Cube::Owner _player = (Cube::Owner)(forWhom);

   if(row==0)
   {
    	if(column==0)  // top left corner
	{
	   cubes[0][1]->increase(_player);
	   cubes[1][0]->increase(_player);
	   return;
	}
	else if(column==dim()-1)  // top right corner
	{
	   cubes[0][dim()-2]->increase(_player);
	   cubes[1][dim()-1]->increase(_player);
	   return;
	}
	else  // top edge
	{
	   cubes[0][column-1]->increase(_player);
	   cubes[0][column+1]->increase(_player);
	   cubes[1][column]->increase(_player);
	   return;
	}
   }
   else if(row==dim()-1)
   {
      if(column==0)  // left bottom corner
      {
         cubes[dim()-2][0]->increase(_player);
         cubes[dim()-1][1]->increase(_player);
         return;
      }

      else if(column==dim()-1) // right bottom corner
      {
         cubes[dim()-2][dim()-1]->increase(_player);
         cubes[dim()-1][dim()-2]->increase(_player);
	      return;
      }
      else  // bottom edge
      {
 	      cubes[dim()-1][column-1]->increase(_player);
	      cubes[dim()-1][column+1]->increase(_player);
	      cubes[dim()-2][column]->increase(_player);
	      return;
      }
   }
   else if(column==0) // left edge
   {
      cubes[row-1][0]->increase(_player);
      cubes[row+1][0]->increase(_player);
      cubes[row][1]->increase(_player);
      return;
   }
   else if(column==dim()-1)  // right edge
   {
      cubes[row-1][dim()-1]->increase(_player);
      cubes[row+1][dim()-1]->increase(_player);
      cubes[row][dim()-2]->increase(_player);
      return;
   }
   else
   {
      cubes[row][column-1]->increase(_player);
      cubes[row][column+1]->increase(_player);
      cubes[row-1][column]->increase(_player);
      cubes[row+1][column]->increase(_player);
      return;
   }


}
