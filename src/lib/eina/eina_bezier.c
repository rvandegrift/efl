/* EINA - EFL data type library
 * Copyright (C) 2015 Subhransu Mohanty
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */
#include "eina_private.h"
#include "eina_bezier.h"

#include <math.h>
#include <float.h>

#define FLOAT_CMP(a, b) (fabs(a - b) <= 0.01/* DBL_MIN */)

static void
_eina_bezier_1st_derivative(const Eina_Bezier *bz,
                            double t,
                            double *px, double *py)
{
   // p'(t) = 3 * (-(1-2t+t^2) * p0 + (1 - 4 * t + 3 * t^2) * p1 + (2 * t - 3 * t^2) * p2 + t^2 * p3)

   double m_t = 1. - t;

   double d = t * t;
   double a = -m_t * m_t;
   double b = 1 - 4 * t + 3 * d;
   double c = 2 * t - 3 * d;

   *px = 3 * ( a * bz->start.x + b * bz->ctrl_start.x + c * bz->ctrl_end.x + d * bz->end.x);
   *py = 3 * ( a * bz->start.y + b * bz->ctrl_start.y + c * bz->ctrl_end.y + d * bz->end.y);
}

// approximate sqrt(x*x + y*y) using alpha max plus beta min algorithm.
// With alpha = 1, beta = 3/8, giving results with a largest error less
// than 7% compared to the exact value.
static double
_line_length(double x1, double y1, double x2, double y2)
{
   double x = x2 - x1;
   double y = y2 - y1;

   x = x < 0 ? -x : x;
   y = y < 0 ? -y : y;

   return (x > y ? x + 0.375 * y : y + 0.375 * x);
}

static void
_eina_bezier_split(const Eina_Bezier *b,
                   Eina_Bezier *first, Eina_Bezier *second)
{
   double c = (b->ctrl_start.x + b->ctrl_end.x) * 0.5;

   first->ctrl_start.x = (b->start.x + b->ctrl_start.x) * 0.5;
   second->ctrl_end.x = (b->ctrl_end.x + b->end.x) * 0.5;
   first->start.x = b->start.x;
   second->end.x = b->end.x;
   first->ctrl_end.x = (first->ctrl_start.x + c) * 0.5;
   second->ctrl_start.x = (second->ctrl_end.x + c) * 0.5;
   first->end.x = second->start.x = (first->ctrl_end.x + second->ctrl_start.x) * 0.5;

   c = (b->ctrl_start.y + b->ctrl_end.y) / 2;
   first->ctrl_start.y = (b->start.y + b->ctrl_start.y) * 0.5;
   second->ctrl_end.y = (b->ctrl_end.y + b->end.y) * 0.5;
   first->start.y = b->start.y;
   second->end.y = b->end.y;
   first->ctrl_end.y = (first->ctrl_start.y + c) * 0.5;
   second->ctrl_start.y = (second->ctrl_end.y + c) * 0.5;
   first->end.y = second->start.y = (first->ctrl_end.y + second->ctrl_start.y) * 0.5;
}

static void
_eina_bezier_length_helper(const Eina_Bezier *b,
                           double *length)
{
   Eina_Bezier left, right; /* bez poly splits */
   double len = 0.0; /* arc length */
   double chord; /* chord length */

   len = len + _line_length(b->start.x, b->start.y, b->ctrl_start.x, b->ctrl_start.y);
   len = len + _line_length(b->ctrl_start.x, b->ctrl_start.y, b->ctrl_end.x, b->ctrl_end.y);
   len = len + _line_length(b->ctrl_end.x, b->ctrl_end.y, b->end.x, b->end.y);

   chord = _line_length(b->start.x, b->start.y, b->end.x, b->end.y);

   if (!FLOAT_CMP(len, chord)) {
      _eina_bezier_split(b, &left, &right);       /* split in two */
      _eina_bezier_length_helper(&left, length);  /* try left side */
      _eina_bezier_length_helper(&right, length); /* try right side */
      return;
   }

   *length = *length + len;

   return;
}

EAPI void
eina_bezier_values_set(Eina_Bezier *b,
                       double start_x, double start_y,
                       double ctrl_start_x, double ctrl_start_y,
                       double ctrl_end_x, double ctrl_end_y,
                       double end_x, double end_y)
{
   b->start.x = start_x;
   b->start.y = start_y;
   b->ctrl_start.x = ctrl_start_x;
   b->ctrl_start.y = ctrl_start_y;
   b->ctrl_end.x = ctrl_end_x;
   b->ctrl_end.y = ctrl_end_y;
   b->end.x = end_x;
   b->end.y = end_y;
}

EAPI void
eina_bezier_values_get(const Eina_Bezier *b,
                       double *start_x, double *start_y,
                       double *ctrl_start_x, double *ctrl_start_y,
                       double *ctrl_end_x, double *ctrl_end_y,
                       double *end_x, double *end_y)
{
   if (start_x) *start_x = b->start.x;
   if (start_y) *start_y = b->start.y;
   if (ctrl_start_x) *ctrl_start_x = b->ctrl_start.x;
   if (ctrl_start_y) *ctrl_start_y = b->ctrl_start.y;
   if (ctrl_end_x) *ctrl_end_x = b->ctrl_end.x;
   if (ctrl_end_y) *ctrl_end_y = b->ctrl_end.y;
   if (end_x) *end_x = b->end.x;
   if (end_y) *end_y = b->end.y;
}


EAPI void
eina_bezier_point_at(const Eina_Bezier *bz, double t, double *px, double *py)
{
   double m_t = 1.0 - t;

   if (px)
     {
        double a = bz->start.x * m_t + bz->ctrl_start.x * t;
        double b = bz->ctrl_start.x * m_t + bz->ctrl_end.x * t;
        double c = bz->ctrl_end.x * m_t + bz->end.x * t;

        a = a * m_t + b * t;
        b = b * m_t + c * t;
        *px = a * m_t + b * t;
     }

   if (py)
     {
        double a = bz->start.y * m_t + bz->ctrl_start.y * t;
        double b = bz->ctrl_start.y * m_t + bz->ctrl_end.y * t;
        double c = bz->ctrl_end.y * m_t + bz->end.y * t;

        a = a * m_t + b * t;
        b = b * m_t + c * t;
        *py = a * m_t + b * t;
     }
}

EAPI double
eina_bezier_angle_at(const Eina_Bezier *b, double t)
{
   double x, y;
   double theta;
   double theta_normalized;

   _eina_bezier_1st_derivative(b, t, &x, &y);
   theta = atan2(y, x) * 360.0 / (M_PI * 2);
   theta_normalized = theta < 0 ? theta + 360 : theta;

   return theta_normalized;
}

EAPI double
eina_bezier_length_get(const Eina_Bezier *b)
{
   double length = 0.0;

   _eina_bezier_length_helper(b, &length);

   return length;
}

static void
_eina_bezier_split_left(Eina_Bezier *b, double t, Eina_Bezier *left)
{
   Eina_Bezier local;

   if (!left) left = &local;

   left->start.x = b->start.x;
   left->start.y = b->start.y;

   left->ctrl_start.x = b->start.x + t * ( b->ctrl_start.x - b->start.x );
   left->ctrl_start.y = b->start.y + t * ( b->ctrl_start.y - b->start.y );

   left->ctrl_end.x = b->ctrl_start.x + t * ( b->ctrl_end.x - b->ctrl_start.x ); // temporary holding spot
   left->ctrl_end.y = b->ctrl_start.y + t * ( b->ctrl_end.y - b->ctrl_start.y ); // temporary holding spot

   b->ctrl_end.x = b->ctrl_end.x + t * ( b->end.x - b->ctrl_end.x );
   b->ctrl_end.y = b->ctrl_end.y + t * ( b->end.y - b->ctrl_end.y );

   b->ctrl_start.x = left->ctrl_end.x + t * ( b->ctrl_end.x - left->ctrl_end.x);
   b->ctrl_start.y = left->ctrl_end.y + t * ( b->ctrl_end.y - left->ctrl_end.y);

   left->ctrl_end.x = left->ctrl_start.x + t * ( left->ctrl_end.x - left->ctrl_start.x );
   left->ctrl_end.y = left->ctrl_start.y + t * ( left->ctrl_end.y - left->ctrl_start.y );

   left->end.x = b->start.x = left->ctrl_end.x + t * (b->ctrl_start.x - left->ctrl_end.x);
   left->end.y = b->start.y = left->ctrl_end.y + t * (b->ctrl_start.y - left->ctrl_end.y);
}

EAPI double
eina_bezier_t_at(const Eina_Bezier *b, double l)
{
   double len = eina_bezier_length_get(b);
   double biggest = 1.0;
   double t = 1.0;

   if (l > len || (FLOAT_CMP(len, l)))
     return t;

   t *= 0.5;

   while (1)
     {
        Eina_Bezier right = *b;
        Eina_Bezier left;
        double ll;

        _eina_bezier_split_left(&right, t, &left);
        ll = eina_bezier_length_get(&left);

        if (FLOAT_CMP(ll, l))
          break;

        if (ll < l)
          {
             t += (biggest - t) * 0.5;
          }
        else
          {
             biggest = t;
             t -= t * 0.5;
          }
     }

   return t;
}

EAPI void
eina_bezier_split_at_length(const Eina_Bezier *b, double len,
                            Eina_Bezier *left, Eina_Bezier *right)
{
   Eina_Bezier local;
   double t;

   if (!right) right = &local;
   *right = *b;

   t =  eina_bezier_t_at(right, len);
   _eina_bezier_split_left(right, t, left);
}
