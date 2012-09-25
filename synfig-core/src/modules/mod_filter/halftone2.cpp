/* === S Y N F I G ========================================================= */
/*!	\file halftone2.cpp
**	\brief Implementation of the "Halftone 2" layer
**
**	$Id$
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**	Copyright (c) 2007-2008 Chris Moore
**
**	This package is free software; you can redistribute it and/or
**	modify it under the terms of the GNU General Public License as
**	published by the Free Software Foundation; either version 2 of
**	the License, or (at your option) any later version.
**
**	This package is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
**	General Public License for more details.
**	\endlegal
*/
/* ========================================================================= */

/* === H E A D E R S ======================================================= */

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "halftone2.h"
#include "halftone.h"

#include <synfig/string.h>
#include <synfig/time.h>
#include <synfig/context.h>
#include <synfig/paramdesc.h>
#include <synfig/renddesc.h>
#include <synfig/surface.h>
#include <synfig/value.h>
#include <synfig/valuenode.h>

#endif

/* === M A C R O S ========================================================= */

using namespace synfig;
using namespace std;
using namespace etl;

/* === G L O B A L S ======================================================= */

SYNFIG_LAYER_INIT(Halftone2);
SYNFIG_LAYER_SET_NAME(Halftone2,"halftone2");
SYNFIG_LAYER_SET_LOCAL_NAME(Halftone2,N_("Halftone 2"));
SYNFIG_LAYER_SET_CATEGORY(Halftone2,N_("Filters"));
SYNFIG_LAYER_SET_VERSION(Halftone2,"0.0");
SYNFIG_LAYER_SET_CVS_ID(Halftone2,"$Id$");

/* === P R O C E D U R E S ================================================= */

/* === M E T H O D S ======================================================= */

Halftone2::Halftone2():
	color_dark(Color::black()),
	color_light(Color::white())
{
	halftone.origin=(synfig::Point(0,0));
	halftone.size=(synfig::Vector(0.25,0.25));
	halftone.angle=(Angle::zero());
	halftone.type=TYPE_SYMMETRIC;

	set_blend_method(Color::BLEND_STRAIGHT);

	Layer::Vocab voc(get_param_vocab());
	Layer::fill_static(voc);
}

inline Color
Halftone2::color_func(const Point &point, float supersample,const Color& color)const
{
	const float amount(halftone(point,color.get_y(),supersample));
	Color halfcolor;

	if(amount<=0.0f)
		halfcolor=color_dark;
	else if(amount>=1.0f)
		halfcolor=color_light;
	else
		halfcolor=Color::blend(color_light,color_dark,amount,Color::BLEND_STRAIGHT);

	halfcolor.set_a(color.get_a());

	return halfcolor;
}

inline float
Halftone2::calc_supersample(const synfig::Point &/*x*/, float pw,float /*ph*/)const
{
	return abs(pw/(halftone.size).mag());
}

synfig::Layer::Handle
Halftone2::hit_check(synfig::Context /*context*/, const synfig::Point &/*point*/)const
{
	return const_cast<Halftone2*>(this);
}

bool
Halftone2::set_param(const String & param, const ValueBase &value)
{
	IMPORT(color_dark);
	IMPORT(color_light);

	IMPORT_AS(halftone.size,"size");
	IMPORT_AS(halftone.type,"type");
	IMPORT_AS(halftone.angle,"angle");
	IMPORT_AS(halftone.origin,"origin");

	IMPORT_AS(halftone.origin,"offset");

	return Layer_Composite::set_param(param,value);
}

ValueBase
Halftone2::get_param(const String & param)const
{
	EXPORT_AS(halftone.size,"size");
	EXPORT_AS(halftone.type,"type");
	EXPORT_AS(halftone.angle,"angle");
	EXPORT_AS(halftone.origin,"origin");

	EXPORT(color_dark);
	EXPORT(color_light);

	EXPORT_NAME();
	EXPORT_VERSION();

	return Layer_Composite::get_param(param);
}

Layer::Vocab
Halftone2::get_param_vocab()const
{
	Layer::Vocab ret(Layer_Composite::get_param_vocab());

	ret.push_back(ParamDesc("origin")
		.set_local_name(_("Mask Origin"))
		.set_is_distance()
	);
	ret.push_back(ParamDesc("angle")
		.set_local_name(_("Mask Angle"))
		.set_origin("origin")
	);
	ret.push_back(ParamDesc("size")
		.set_local_name(_("Mask Size"))
		.set_is_distance()
		.set_origin("origin")
	);
	ret.push_back(ParamDesc("color_light")
		.set_local_name(_("Light Color"))
	);
	ret.push_back(ParamDesc("color_dark")
		.set_local_name(_("Dark Color"))
	);
	ret.push_back(ParamDesc("type")
		.set_local_name(_("Type"))
		.set_hint("enum")
		.add_enum_value(TYPE_SYMMETRIC,"symmetric",_("Symmetric"))
		.add_enum_value(TYPE_LIGHTONDARK,"lightondark",_("Light On Dark"))
		//.add_enum_value(TYPE_DARKONLIGHT,"darkonlight",_("Dark on Light"))
		.add_enum_value(TYPE_DIAMOND,"diamond",_("Diamond"))
		.add_enum_value(TYPE_STRIPE,"stripe",_("Stripe"))
	);

	return ret;
}

Color
Halftone2::get_color(Context context, const Point &point)const
{
	const Color undercolor(context.get_color(point));
	const Color color(color_func(point,0,undercolor));

	if(get_amount()==1.0 && get_blend_method()==Color::BLEND_STRAIGHT)
		return color;
	else
		return Color::blend(color,undercolor,get_amount(),get_blend_method());
}

bool
Halftone2::accelerated_render(Context context,Surface *surface,int quality, const RendDesc &renddesc, ProgressCallback *cb)const
{
	SuperCallback supercb(cb,0,9500,10000);

	if(!context.accelerated_render(surface,quality,renddesc,&supercb))
		return false;
	if(get_amount()==0)
		return true;

	const Real pw(renddesc.get_pw()),ph(renddesc.get_ph());
	const Point tl(renddesc.get_tl());
	const int w(surface->get_w());
	const int h(surface->get_h());
	const float supersample_size(abs(pw/(halftone.size).mag()));

	Surface::pen pen(surface->begin());
	Point pos;
	int x,y;

	if(is_solid_color())
	{
		for(y=0,pos[1]=tl[1];y<h;y++,pen.inc_y(),pen.dec_x(x),pos[1]+=ph)
			for(x=0,pos[0]=tl[0];x<w;x++,pen.inc_x(),pos[0]+=pw)
				pen.put_value(
					color_func(
						pos,
						supersample_size,
						pen.get_value()
					)
				);
	}
	else
	{
		for(y=0,pos[1]=tl[1];y<h;y++,pen.inc_y(),pen.dec_x(x),pos[1]+=ph)
			for(x=0,pos[0]=tl[0];x<w;x++,pen.inc_x(),pos[0]+=pw)
				pen.put_value(
					Color::blend(
				 		color_func(
							pos,
							supersample_size,
							pen.get_value()
						),
						pen.get_value(),
						get_amount(),
						get_blend_method()
					)
				);
	}

	// Mark our progress as finished
	if(cb && !cb->amount_complete(10000,10000))
		return false;

	return true;
}


///
bool
Halftone2::accelerated_cairorender(Context context,cairo_surface_t *surface,int quality, const RendDesc &renddesc, ProgressCallback *cb)const
{
	SuperCallback supercb(cb,0,9500,10000);
	
	if(!context.accelerated_cairorender(surface,quality,renddesc,&supercb))
		return false;
	if(get_amount()==0)
		return true;
	
	CairoSurface csurface(surface);
	if(!csurface.map_cairo_image())
		return false;
	const Real pw(renddesc.get_pw()),ph(renddesc.get_ph());
	const Point tl(renddesc.get_tl());
	const int w(csurface.get_w());
	const int h(csurface.get_h());
	const float supersample_size(abs(pw/(halftone.size).mag()));
	
	CairoSurface::pen pen(csurface.begin());
	Point pos;
	int x,y;
	
	if(is_solid_color())
	{
		for(y=0,pos[1]=tl[1];y<h;y++,pen.inc_y(),pen.dec_x(x),pos[1]+=ph)
			for(x=0,pos[0]=tl[0];x<w;x++,pen.inc_x(),pos[0]+=pw)
				pen.put_value(
					CairoColor(color_func(
										  pos,
										  supersample_size,
										  Color(pen.get_value().demult_alpha())
										 )
					).premult_alpha()
				);
	}
	else
	{
		for(y=0,pos[1]=tl[1];y<h;y++,pen.inc_y(),pen.dec_x(x),pos[1]+=ph)
			for(x=0,pos[0]=tl[0];x<w;x++,pen.inc_x(),pos[0]+=pw)
			{
				Color val=Color(pen.get_value().demult_alpha());
				pen.put_value(
						CairoColor(Color::blend(
											  color_func(
														 pos,
														 supersample_size,
														 val
														 ),
											  val,
											  get_amount(),
											  get_blend_method()
											  ).clamped()
						).premult_alpha()
				);
			}
				
	}

	csurface.unmap_cairo_image();
	// Mark our progress as finished
	if(cb && !cb->amount_complete(10000,10000))
		return false;
	
	return true;
}

///