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

#include "ai_kepler.h"
#include "cube.h"

AI_Kepler::AI_Kepler()
{
}

int AI_Kepler::assessCube(int row,int column,CubeBox::Player player,CubeBox& box) const
{
   int diff;

   // qDebug() << "assessCube(" << row << column << "player" << player;
   if(row==0)  // first row
   {
      if(column == 0)  // upper left corner
      {
         diff=getDiff(0,1,player,box) ;
         int temp=getDiff(1,0,player,box);
         if(temp < diff)
            diff=temp;
      }
      else if(column == box.dim()-1) // upper right corner
      {
         diff=getDiff(0,column-1,player,box);
         int temp=getDiff(1,column,player,box);
         if(temp < diff)
	    diff=temp;
      }
      else
      {
         diff=getDiff(row,column-1,player,box);
         int temp=getDiff(row,column+1,player,box);
         if(temp < diff)
            diff = temp;
         temp=getDiff(row+1,column,player,box);
         if(temp < diff)
            diff = temp;
      }
   }
   else if(row==box.dim()-1) // last row
   {
      if(column == 0) // lower left corner
      {
         diff=getDiff(row,1,player,box);
         int temp=getDiff(row-1,0,player,box);
         if(temp < diff)
            diff=temp;
      }
      else if(column == box.dim()-1) // lower right corner
      {
         diff=getDiff(row,column-1,player,box);
         int temp=getDiff(row-1,column,player,box);
         if(temp < diff)
            diff=temp;
      }
      else
      {
         diff=getDiff(row,column-1,player,box);
         int temp=getDiff(row,column+1,player,box);
         if(temp < diff)
            diff = temp;
         temp=getDiff(row-1,column,player,box);
         if(temp < diff)
            diff = temp;
      }
   }
   else if(column == 0) // first column
   {
       diff = getDiff(row,1,player,box);
       int temp = getDiff(row-1,0,player,box);
       if(temp < diff)
          diff = temp;
       temp = getDiff(row+1,0,player,box);
       if(temp < diff)
          diff = temp;
   }
   else if(column == box.dim()-1) // last column
   {
      diff = getDiff(row,column-1,player,box);
      int temp = getDiff(row-1,column,player,box);
      if(temp < diff)
         diff = temp;
      temp = getDiff(row+1,column,player,box);
      if(temp < diff)
         diff = temp;
   }
   else
   {
      diff=getDiff(row-1,column,player,box);
      int temp=getDiff(row+1,column,player,box);
      if(temp < diff)
         diff = temp;
      temp=getDiff(row,column-1,player,box);
      if(temp < diff)
         diff = temp;
      temp=getDiff(row,column+1,player,box);
      if(temp < diff)
         diff = temp;
   }

   int temp;
   temp=( box[row][column]->max()-box[row][column]->value() );

   int val;
   val=diff-temp+1;
   val=val*(temp+1);

   return val;
}

double AI_Kepler::assessField (CubeBox::Player player, CubeBox& box) const
{
   int    cubesOne       = 0;
   int    cubesTwo       = 0;
   int    pointsOne      = 0;
   int    pointsTwo      = 0;
   CubeBox::Player otherPlayer    = (player == CubeBox::One) ?
                                     CubeBox::Two : CubeBox::One;
   bool   playerWon      = true;
   bool   otherPlayerWon = true;

   int d = box.dim();
   int i, j;

   for (i = 0; i < d; i++) {
      for (j = 0; j < d; j++) {
         Cube * cube = box[i][j];
	 int points  = cube->value();
         if (cube->owner() == (Cube::Owner)CubeBox::One) {
            cubesOne++;
            pointsOne += points * points;
         }
         else if (cube->owner() == (Cube::Owner)CubeBox::Two) {
            cubesTwo++;
	    pointsTwo += points * points;
         }

         if(cube->owner() != (Cube::Owner)player)
            playerWon = false;

         if(cube->owner() != (Cube::Owner)otherPlayer)
            otherPlayerWon = false;
      }
   }

   if (player == CubeBox::One) {
      return cubesOne * cubesOne + pointsOne - cubesTwo * cubesTwo - pointsTwo;
   }
   else {
      return cubesTwo * cubesTwo + pointsTwo - cubesOne * cubesOne - pointsOne;
   }
}

int AI_Kepler::getDiff(int row,int column, CubeBox::Player player, CubeBox& box) const
{
   int diff;

   if (box[row][column]->owner() != (Cube::Owner)player) {
      diff = (box[row][column]->max() - box[row][column]->value());
   }
   else {
      diff = (box[row][column]->max() - box[row][column]->value() + 1);
   }

   return diff;
}
