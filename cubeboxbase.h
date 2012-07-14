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
#ifndef CUBEBOXBASE_H
#define CUBEBOXBASE_H

#ifdef DEBUG
#include <assert.h>
#endif

template<class T> 
class CubeBoxBase
{
public:
   enum Player{One=1,Two=2};
  
   CubeBoxBase(const int dim=1);
   virtual ~CubeBoxBase();
   
   T** operator[](const int index);
   /**
   * sets number of Cubes in a row/column to 'size'.
   */
   virtual void setDim(int dim);
   
   /**
   * returns number of Cubes in a row/column
   */
   inline int dim() const { return _dim; }
   
   inline Player player() const { return currentPlayer; } 
   T*** cubes; // IDW test.
   
protected:
   virtual void deleteCubes();
   virtual void initCubes();
   
   /** increases the neighbours of cube at ['row','column'] */
   //void increaseNeighbours(int forWhom,int row,int column);
  
   // IDW test. T*** cubes;
   Player currentPlayer;
   
private:
   int _dim;
         
};

template<class T>
CubeBoxBase<T>::CubeBoxBase(const int dim)
{
#ifdef DEBUG
   assert(dim>0);
#endif
   
   _dim=dim;
   currentPlayer=One;
}

template<class T>
CubeBoxBase<T>::~CubeBoxBase()
{
   if(cubes)
      deleteCubes();
}

template<class T>
T** CubeBoxBase<T>::operator[](const int index)
{
#ifdef DEBUG
   assert(index >= 0);
#endif

    return cubes[index];
}
   
template<class T>
void CubeBoxBase<T>::setDim(int d)
{
   if(d != _dim)
   {
      deleteCubes();
    
      _dim=d;     
      
      initCubes();
   }
}


template<class T>
void CubeBoxBase<T>::initCubes()
{
   const int s=dim();
   
   int i,j;
   // create new cubes
   cubes = new T**[s];
   for(i=0;i<s;i++)
   {
      cubes[i]=new T*[s];
   }
   for(i=0;i<s;i++)
      for(j=0;j<s;j++)
      {
         cubes[i][j]=new T();
      }
   
   // initialize cubes  
   int max=dim()-1;
      
   cubes[0][0]->setMax(2);
   cubes[0][max]->setMax(2);
   cubes[max][0]->setMax(2);
   cubes[max][max]->setMax(2);
   
   // IDW TODO - This is WRONG: should be "i=1;". But this copy of initCubes()
   // is probably not used: there is an initCubes() in KCubeBoxWidget, which
   // DOES set the cubes' max values correctly.
   for(i=0;i<=max;i++)
   {
      cubes[i][0]->setMax(3);
      cubes[i][max]->setMax(3);
      cubes[0][i]->setMax(3);
      cubes[max][i]->setMax(3);
   }
   
   for(i=1;i<max;i++)
     for(j=1;j<max;j++)
      {
         cubes[i][j]->setMax(4);
      }
}

template<class T>
void CubeBoxBase<T>::deleteCubes()
{  
   int i,j;
   for(i=0;i<dim();i++)
      for(j=0;j<dim();j++)
      {
         delete cubes[i][j];
      }
   for(i=0;i<dim();i++)
   {
      delete [] cubes[i];
   }
      
   delete [] cubes;
      
   cubes=0;
}

#endif // CUBEBOXBASE_H

