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

#include "cubebox.h"
#include "kcubeboxwidget.h"

#include <assert.h>
#include <math.h>
#include <QDebug> // IDW test.

CubeBox::CubeBox(const int d)
    :CubeBoxBase<Cube>(d)
{
   qDebug() << "CONSTRUCT CubeBox, size" << d; // IDW test.
   initCubes();
}

CubeBox::CubeBox(const CubeBox& box)
      :CubeBoxBase<Cube>(box.dim())
{
   initCubes();
   // qDebug() << "COPY CubeBox, size" << dim(); // IDW test.

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
   qDebug() << "COPY KCubeBoxWidget, size" << dim(); // IDW test.

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
   // qDebug() << "DESTROY CubeBox"; // IDW test.
}

/* IDW test.  Redundant?  YES!!
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
End IDW test. */


CubeBox& CubeBox::operator=(KCubeBoxWidget& box)
{
   qDebug() << "OPERATOR = KCubeBoxWidget, size" << dim(); // IDW test.
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



/*
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

   if(row!=0)
     cubes[row-1][column]->increase(_player);
   if(row!=dim()-1)
     cubes[row+1][column]->increase(_player);
   if(column!=0)
     cubes[row][column-1]->increase(_player);
   if(column!=dim()-1)
     cubes[row][column+1]->increase(_player);
}
*/
