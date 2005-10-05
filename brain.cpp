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

#include "brain.h"
#include "cube.h"

#include <math.h>

#include <kapplication.h>

#undef DEBUG // uncomment this to get useful messages
#include <assert.h>

#ifdef DEBUG
#include <iostream.h>
#endif

#include "prefs.h"

Brain::Brain(int initValue)
{
   setSkill(Prefs::EnumSkill::Beginner);
   stopped=false;
   active=false;
   currentLevel=0;

   // initialize the random number generator
   random.setSeed(initValue);
}

void Brain::setSkill(int newSkill)
{
   _skill=newSkill;

   switch(_skill)
   {
      case Prefs::EnumSkill::Beginner:
         maxLevel=1;
	 break;
      case Prefs::EnumSkill::Average:
         maxLevel=3;
         break;
      case Prefs::EnumSkill::Expert:
         maxLevel=5;
         break;
      default:
         break;
   }
}

int Brain::skill() const
{
   return _skill;
}

void Brain::stop()
{
   stopped=true;
}


bool Brain::isActive()  const
{
   return active;
}



bool Brain::getHint(int& row, int& column,CubeBox::Player player ,CubeBox box)
{
   if(isActive())
      return false;

   active=true;
   stopped=false;
   currentPlayer=player;

   int i=0,j=0;
   int moves=0; // how many moves are the favourable ones
   CubeBox::Player opponent=(player==CubeBox::One)?CubeBox::Two : CubeBox::One;

   // if more than one cube has the same rating this array is used to select
   // one
   coordinate* c2m=new coordinate[box.dim()*box.dim()];

   // Array, which holds the assessment of the separate moves
   double **worth=new double*[box.dim()];
   for(i=0;i<box.dim();i++)
      worth[i]=new double[box.dim()];

   // alle Werte auf kleinstmöglichen Wert setzen
   double min=-pow(2.0,sizeof(long int)*8-1);  // Maximum auf kleinst möglichen Wert setzen

   for(i=0;i<box.dim();i++)
     for(j=0;j<box.dim();j++)
     {
        worth[i][j]=min;
     }


   // find the favourable cubes to increase
   moves=findCubes2Move(c2m,player,box);


   // if only one cube is found, then don't check recursively the move
   if(moves==1)
   {
#ifdef DEBUG
      cerr << "found only one favourable cube" << endl;
#endif
      row=c2m[0].row;
      column=c2m[0].column;
   }
   else
   {
#ifdef DEBUG
      cerr << "found more than one favourable cube: " << moves << endl;
#endif
      for(i=0;i<moves;i++)
      {
	 // if Thinking process stopped
	 if(stopped)
	 {
#ifdef DEBUG
	     cerr << "brain stopped" << endl;
#endif
	     active=false;
             for(i=0;i<box.dim();i++)
                delete[] worth[i];
             delete [] worth;

             delete [] c2m;

	     return false;
	 }

#ifdef DEBUG
	 cerr << "checking cube " << c2m[i].row << "," << c2m[i].column << endl;
#endif
         // for every found possible move, simulate this move and store the assessment
	 worth[c2m[i].row][c2m[i].column]=doMove(c2m[i].row,c2m[i].column,player,box);

#ifdef DEBUG
	 cerr << "cube "  << c2m[i].row << "," << c2m[i].column << " : " << worth[c2m[i].row][c2m[i].column] << endl;
#endif
      }


      // find the maximum
      double max=-1E99;  // set max to minimum value

#ifdef DEBUG
      cerr << "searching for the maximum" << endl;
#endif

      for(i=0;i<moves;i++)
      {
         if(box[c2m[i].row][c2m[i].column]->owner()!=(Cube::Owner)opponent)
         {
            if(worth[c2m[i].row][c2m[i].column]>max )
            {
               max=worth[c2m[i].row][c2m[i].column];
            }
         }
      }

#ifdef DEBUG
      cerr << "found Maximum : " << max << endl;
#endif

      // found maximum more than one time ?
      int counter=0;
      for(i=0;i<moves;i++)
      {
#ifdef DEBUG
         cerr << c2m[i].row << "," << c2m[i].column << " : " << worth[c2m[i].row][c2m[i].column] << endl;
#endif
         if(worth[c2m[i].row][c2m[i].column]==max)
            if(box[c2m[i].row][c2m[i].column]->owner() != (Cube::Owner)opponent)
	    {
	       c2m[counter].row=c2m[i].row;
	       c2m[counter].column=c2m[i].column;
	       counter++;
	    }
      }

      assert(counter>0);


      // if some moves are equal, choose a random one
      if(counter>1)
      {

#ifdef DEBUG
         cerr << "choosing a random cube: " << endl ;
#endif
         counter=random.getLong(counter);
      }

      row=c2m[counter].row;
      column=c2m[counter].column;
#ifdef DEBUG
      cerr << "cube: " << row << "," << column << endl;
#endif
   }

   // clean up
   for(i=0;i<box.dim();i++)
      delete[] worth[i];
   delete [] worth;

   delete [] c2m;

   active=false;

   return true;
}



double Brain::doMove(int row, int column, CubeBox::Player player , CubeBox box)
{
   double worth=0;
   currentLevel++; // increase the current depth of recurse calls


   // if the maximum depth isn't reached
   if(currentLevel < maxLevel)
   {
       // test, if possible to increase this cube
      if(!box.simulateMove(player,row,column))
      {
         currentLevel--;
         return 0;
      }

      // if the player has won after simulating this move, return the assessment of the field
      if(box.playerWon(player))
      {
         currentLevel--;

	 return (long int)pow((float)box.dim()*box.dim(),(maxLevel-currentLevel))*box.assessField(currentPlayer);
      }


      int i;
      int moves=0;
      // if more than one cube has the same rating this array is used to select
      // one
      coordinate* c2m=new coordinate[box.dim()*box.dim()];

      // the next move has does the other player
      player=(player==CubeBox::One)? CubeBox::Two : CubeBox::One;

      // find the favourable cubes to increase
      moves=findCubes2Move(c2m,player,box);

      // if only one cube is found, then don't check recursively the move
      if(moves==1)
      {
         box.simulateMove(player,c2m[0].row,c2m[0].column);
         worth=(long int)pow((float)box.dim()*box.dim(),(maxLevel-currentLevel-1))*box.assessField(currentPlayer);
      }
      else
      {
         for(i=0;i<moves;i++)
         {
            kapp->processEvents();

	    // if thinking process stopped
	    if(stopped)
	    {
	       currentLevel--;
	       return 0;
	    }

	    // simulate every possible move
	    worth+=doMove(c2m[i].row,c2m[i].column,player,box);
         }
      }
      delete [] c2m;
      currentLevel--;
      return worth;

   }
   else
   {
      // if maximum depth of recursive calls are reached, return the assessment
      currentLevel--;
      box.simulateMove(player,row,column);

      return box.assessField(currentPlayer);
   }

}

int Brain::findCubes2Move(coordinate *c2m,CubeBox::Player player,CubeBox& box)
{
   int i,j;
   int opponent=(player==CubeBox::One)? CubeBox::Two : CubeBox::One;
   int moves=0;
   int min=9999;

   if(_skill==Prefs::EnumSkill::Beginner)
   {
      int max=0;
      for(i=0;i<box.dim();i++)
        for(j=0;j<box.dim();j++)
        {
           if(box[i][j]->owner() != opponent)
           {
              c2m[moves].row=i;
              c2m[moves].column=j;
              c2m[moves].val=box[i][j]->value();

              if(c2m[moves].val>max)
                 max=c2m[moves].val;

              moves++;

	   }
        }

    // find all moves with maximum value
    int counter=0;
    for(i=0;i<moves;i++)
    {
       if(c2m[i].val==max)
       {
	  c2m[counter].row=c2m[i].row;
	  c2m[counter].column=c2m[i].column;
	  c2m[counter].val=c2m[i].val;

          counter++;
        }
     }

     if(counter!=0)
     {
        moves=counter;
     }
   }
   else // if skill is not Beginner
   {
      int secondMin=min;
      // put values on the cubes
      for(i=0;i<box.dim();i++)
        for(j=0;j<box.dim();j++)
        {
	   // use only cubes, who don't belong to the opponent
	   if(box[i][j]->owner() != opponent)
	   {
	      int val;

	      // check neighbours of every cube
	      val=assessCube(i,j,player,box);


#ifdef DEBUG
	      if(currentLevel==0)
	         cerr << i << "," << j << " : " << val << endl;
#endif
	      // only if val >= 0 its a favourable move
              if( val > 0 )
              {
	         if(val<min)
	         {
	            secondMin=min;
		    min=val;
	         }

	         // store coordinates
	         c2m[moves].row=i;
	         c2m[moves].column=j;
	         c2m[moves].val=val;
                 moves++;
	      }
	   }
        }


	// If all cubes are bad, check all cubes for the next move
	if(moves==0)
	{
	   min=4;
	   for(i=0;i<box.dim();i++)
	      for(j=0;j<box.dim();j++)
	      {
	         if(box[i][j]->owner() != opponent)
		 {
                    c2m[moves].row=i;
                    c2m[moves].column=j;
                    c2m[moves].val=( box[i][j]->max() - box[i][j]->value() );
                    if(c2m[moves].val<min)
                       min=c2m[moves].val;
                    moves++;
		 }
	      }
        }

	int counter=0;
	// find all moves with minimum assessment
	for(i=0;i<moves;i++)
	{
	   if(c2m[i].val==min)
	   {
              c2m[counter].row=c2m[i].row;
              c2m[counter].column=c2m[i].column;
              c2m[counter].val=c2m[i].val;

              counter++;
           }
	   else if(_skill == Prefs::EnumSkill::Average)
	   {
	      if(c2m[i].val == secondMin)
	      {
                 c2m[counter].row=c2m[i].row;
                 c2m[counter].column=c2m[i].column;
                 c2m[counter].val=c2m[i].val;

		 counter++;
	      }
	   }
	}

	if(counter!=0)
	{
	   moves=counter;
	}
   }

   int maxMoves=10;
	// if more than maxMoves moves are favourable, take maxMoves random moves
	// because it will take to much time if you check all
	if(moves > maxMoves)
	{
	   // find maxMoves random cubes to move with
	   coordinate* tempC2M=new coordinate[maxMoves];

	   coordinate tmp={-1,-1,0};
	   for(i=0;i<maxMoves;i++)
              tempC2M[i]=tmp;

	   // this array takes the random chosen numbers, so that no
	   // number will be taken two times
	   int *results=new int[moves];
	   for(i=0;i<moves;i++)
	      results[i]=0;

	   for(i=0;i<maxMoves;i++)
	   {
	      int temp;
	      do
	      {
	         temp=random.getLong(moves);
	      }
	      while(results[temp]!=0);

	      results[temp]=1;

	      tempC2M[i].row=c2m[temp].row;
	      tempC2M[i].column=c2m[temp].column;
	      tempC2M[i].val=c2m[temp].val;
	   }
	   delete [] results;

	   for(i=0;i<maxMoves;i++)
	   {
	      c2m[i].row=tempC2M[i].row;
	      c2m[i].column=tempC2M[i].column;
	      c2m[i].val=tempC2M[i].val;
	   }
	   delete [] tempC2M;

	   moves=maxMoves;
	}


   return moves;

}


int Brain::assessCube(int row,int column,CubeBox::Player player,CubeBox& box) const
{
   int diff;

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


int Brain::getDiff(int row,int column, CubeBox::Player player, CubeBox& box) const
{
	int diff;

	if(box[row][column]->owner() != (Cube::Owner)player)
	{
           diff=( box[row][column]->max() - box[row][column]->value() );
	}
	else
	{
           diff=( box[row][column]->max() - box[row][column]->value()+1 );
	}

	return diff;
}

