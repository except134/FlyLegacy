//===============================================================================
// OSMobjects.cpp
//
//
// Part of Fly! Legacy project
//
// Copyright 2005 Chris Wallace
// CopyRight 2007 Jean Sabatier
// Fly! Legacy is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
// Fly! Legacy is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
//   along with Fly! Legacy; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//===============================================================================
#include "../Include/FlyLegacy.h"
#include "../Include/OSMobjects.h"
#include "../Include/RoofModels.h"
#include "../Include/Terraintexture.h"
#include "../Include/fileparser.h"
//===============================================================================
//	Script to create an OSM datatbase from scratch
//===============================================================================
char *ScriptCreateOSM[] = {
	//--- Create QGT table ---------------------------------
	"CREATE TABLE QGT ( key INTEGER UNIQUE);*",
	"CREATE INDEX skey ON QGT(key)",
	//--- Create object table ------------------------------
	"CREATE TABLE OSM_OBJ ("
		"Ident   INTEGER PRIMARY KEY,"		// P01 Object identity
		"QGT		 INTEGER,"								// P02 QGT key
		"Type    INTEGER,"								// P03 Object type
		"Layer   INTEGER,"								// P04 OSM Layer
		"SupNo	 INTEGER,"								// P05 Super Tile number
		"nDir    INTEGER,"								// P06 Texture Directory nO
		"nTex    CHAR(64),"								// P07 Texture name
		"nVtx		 INTEGER,"								// P08 Number of vertices
		"dim     INTEGER,"                // P09 Size of blob
		"Vrtx    BLOB );*",								// P10 blob of vertices
	"CREATE INDEX qkey ON OSM_OBJ(QGT);*",
	//--- Last entry (not a SQL Statement) ------------------
	"***",
};
//===============================================================================
//	Script to create an Airport datatbase from scratch
//===============================================================================
char *ScriptCreateAPO[] = {
	//--- Create QGT table ----------------------------------
	"CREATE TABLE QGT ( key INTEGER UNIQUE);*",
	"CREATE INDEX skey ON QGT(key)",
	//--- Create TEXTURE table ------------------------------
	"CREATE TABLE TEX ("
			"Name CHAR(64),"									// P01 Texture name
			"wd   INTEGER,"										// P02 width
			"ht   INTEGER,"										// P03 height
			"rgba BLOB);*",										// P04 pixel blob
	"CREATE INDEX tkey ON TEX (name);*",
	//--- Create object table ------------------------------
	"CREATE TABLE OBJ ("
			"QGT    INTEGER,"									// P01 QGT key
			"SupNo  INTEGER,"									// P02 SuperTile
			"xlon   INTEGER,"									// P03 decimalized longitude
			"ylat   INTEGER,"									// P04 decimalized latitude
			"Lod    INTEGER,"									// P07 Level of Detail
			"nTex   CHAR(64),"								// P05 Texture name
			"Type   INTEGER,"									// P06 Type of object
			"nVtx   INTEGER,"									// P08 Number of vertices
			"Vrtx   BLOB);*",									// P09 blob of vertices
	"CREATE INDEX okey ON OBJ (QGT, SupNo);*",
	"CREATE UNIQUE INDEX ukey ON OBJ(xlon,ylat);*",
	//--- Last entry (not a SQL Statement) ------------------
	"***",
};
//==========================================================================================
char *EndOSM = "***";
//==========================================================================================
//  Layer name
//==========================================================================================
char *layerNAME[OSM_LAYER_SIZE] = {
		"Buildings",
		"Lights",
		"Forests",
		"Ground",
};
//==========================================================================================
//  List of amenity VALUES Tags
//==========================================================================================
OSM_CONFP amenityVAL[] = {
	//--- TAG VALUE ----OTYPE ----------OPROP ---------		BVEC ----------LAYER-------------------
	{"PLACE_OF_WORSHIP",OSM_CHURCH,			OSM_PROP_BLDG,		OSM_BUILD_BLDG, OSM_LAYER_BLDG},
	{"POLICE",					OSM_FORTIFS,		OSM_PROP_FORTIFS, OSM_BUILD_FORT, OSM_LAYER_DBLE, 2.0, "WALP"},
	{"FIRE_STATION",		OSM_FIRE_STA,		OSM_PROP_IGNR,		OSM_BUILD_BLDG, OSM_LAYER_BLDG},
	{"TOWNHALL",				OSM_TOWNHALL,		OSM_PROP_IGNR,		OSM_BUILD_BLDG, OSM_LAYER_BLDG},
	{"SCHOOL",					OSM_FORTIFS,		OSM_PROP_FORTIFS, OSM_BUILD_FORT, OSM_LAYER_DBLE, 1.5, "HAIE"},
	{"COLLEGE",					OSM_FORTIFS,		OSM_PROP_FORTIFS, OSM_BUILD_FORT, OSM_LAYER_DBLE, 2.0, "HAIE"},
	{"HOSPITAL",				OSM_FORTIFS,		OSM_PROP_FORTIFS, OSM_BUILD_FORT, OSM_LAYER_DBLE, 2.0, "WALP"},
	{EndOSM,					0},									  // End of table
};
//==========================================================================================
//  List of BUILDING VALUES Tags
//==========================================================================================
OSM_CONFP  buildingVAL[] = {
	//--- TAG VALUE --OTYPE ----------OPROP ---------BVEC -----------LAYER -----------
	{"YES",						OSM_BUILDING,		OSM_PROP_BLDG, OSM_BUILD_BLDG, OSM_LAYER_BLDG},
	{"SCHOOL",				OSM_SCHOOL,			OSM_PROP_BLDG, OSM_BUILD_BLDG, OSM_LAYER_BLDG},
	{EndOSM,						0},									// End of table
};
//==========================================================================================
//  List of LIGHT VALUES Tags
//==========================================================================================
OSM_CONFP  liteVAL[] = {
	//--- TAG VALUE --OTYPE ----------OPROP ------------BVEC -----------LAYER ------------
	{"YES",						OSM_LIGHT,			OSM_PROP_LITE,		OSM_BUILD_LITE,		OSM_LAYER_LITE},
	{"RESIDENTIAL",		OSM_LIGHT,			OSM_PROP_LITE,		OSM_BUILD_LITE,		OSM_LAYER_LITE},
	{"PRIMARY",				OSM_PSTREET,		OSM_PROP_PSTREET, OSM_BUILD_PSTR,		OSM_LAYER_DBLE},
	{"SECONDARY",			OSM_PSTREET,		OSM_PROP_PSTREET, OSM_BUILD_PSTR,		OSM_LAYER_DBLE},
	{EndOSM,					0},									// End of table
};
//==========================================================================================
//  List of LAND VALUES Tags
//==========================================================================================
OSM_CONFP	landVAL[] = {
	//--- TAG VALUE --OTYPE ----------OPROP ---------BVEC -----------LAYER ------------
	{"FOREST",				OSM_TREE,				OSM_PROP_TREE, OSM_BUILD_TREE, OSM_LAYER_DBLE},
	{EndOSM,					0},
};
//==========================================================================================
//  List of MAN_MADE VALUES Tags
//==========================================================================================
OSM_CONFP manmadeVAL[] = {
	//--- TAG VALUE --OTYPE ----------OPROP ---------BVEC -----------LAYER ------------
	{"WATER_TOWER",		OSM_CHATODO,		OSM_PROP_BLDG, OSM_BUILD_BLDG, OSM_LAYER_BLDG},
	{"LIGHTHOUSE",    OSM_PHARES,     OSM_PROP_BLDG, OSM_BUILD_BLDG, OSM_LAYER_BLDG},
	{EndOSM,					0},
};
//==========================================================================================
//  List of Junction VALUES Tags
//==========================================================================================
OSM_CONFP junctionVAL[] = {
	//--- TAG VALUE --OTYPE ----------OPROP -----------BVEC -----------LAYER ------------
	{"ROUNDABOUT",		OSM_RPOINT,		  OSM_PROP_RPOINT, OSM_BUILD_GRND, OSM_LAYER_GRND},
	{EndOSM,					0},
};
//==========================================================================================
//  List of barrier VALUES Tags
//==========================================================================================
OSM_CONFP barrierVAL[] = {
	//--- TAG VALUE --OTYPE --------OPROP ----------- BVEC -----------LAYER ------------
	{"CITY_WALL",		  OSM_FORTIFS,	OSM_PROP_FORTIFS, OSM_BUILD_FORT, OSM_LAYER_DBLE, 10, "FORT" },
	{EndOSM,					0},
};
//==========================================================================================
//  List of Waterway VALUES Tags
//==========================================================================================
OSM_CONFP waterwayVAL[] = {
	//--- TAG VALUE --OTYPE ----------OPROP -----------BVEC -----------LAYER ------------
	{EndOSM,					0},
};

//==========================================================================================
//  List of admitted Tags
//==========================================================================================
OSM_TAG TagLIST[] = {
	//--- TAG -----Value Table ---- Layer ---
	{"AMENITY",			amenityVAL,	},
	{"BUILDING",		buildingVAL,},
	{"LIT",					liteVAL,		},
	{"HIGHWAY",			liteVAL,		},
	{"LANDUSE",     landVAL,    },
	{"MAN_MADE",    manmadeVAL,	},
	{"JUNCTION",		junctionVAL,},
	{"BARRIER",			barrierVAL,},
	{EndOSM,					0},									// End of table
};
//==========================================================================================
//  Local rendering vector depending on Layer
//==========================================================================================
OSM_Object::drawCB renderOSM[OSM_LAYER_SIZE] = {
	&OSM_Object::DrawAsBLDG,							// 0 => OSM_LAYER_BLDG
	&OSM_Object::DrawAsLITE,							// 1 => OSM_LAYER_LITE
	&OSM_Object::DrawAsTREE,							// 2 => OSM_LAYER_TREE
	&OSM_Object::DrawAsGRND,							// 3 => OSM_LAYER_GRND

};
//==========================================================================================
//  Vector to write object depending on build vector
//==========================================================================================
OSM_Object::writeCB writeOSM[] = {
	&OSM_Object::SkipWrite,								// 0 =>	OSM_BUILD_GRND										
	&OSM_Object::WriteAsBLDG,							// 1 => OSM_BUILD_BLDG
	&OSM_Object::WriteAsGOSM,							// 2 => OSM_BUILD_LITE
	0,																		// 3 => OSM_BUILD_AMNY
	&OSM_Object::WriteAsGOSM,							// 4 => OSM_BUILD_TREE
	&OSM_Object::WriteAsGOSM,							// 5 => OSM_BUILD_PSTR
	&OSM_Object::WriteAsGOSM,							// 6 =>	OSM_BUILD_FORT
};
//==========================================================================================
//  Street light parameters
//==========================================================================================
float lightOSM[4] = {1,1,float(0.8),1};
U_INT lightDIM    = 64;
float alphaOSM    = float(0.25);
float lightDIS[]  = {0.0f,0.01f,0.00001f};
//==========================================================================================
//	Locate OSM value slot
//==========================================================================================
OSM_CONFP *LocateOSMvalue(char *t,char *v)
{	OSM_TAG *tab = TagLIST;
	while (strcmp(tab->tag,EndOSM) != 0)
	{	if  (strcmp(tab->tag,t)   != 0) {tab++; continue;}
	  OSM_CONFP *cnf = tab->table;	
	  while (strcmp(cnf->val,EndOSM) != 0)
		{	if  (strcmp(cnf->val,v) != 0)	{cnf++; continue; }
			return cnf;
		}
		//--- unknown value --------------------------
		STREETLOG("Unknow value %s %s",t,v);
		return 0;
	}
	//--- Unknown tag ------------------------------
	return 0;
}
//==========================================================================================
//	Locate OSM Tag
//==========================================================================================
OSM_TAG *LocateOSMtag(char *t)
{	OSM_TAG *tab = TagLIST;
	while (strcmp(tab->tag,EndOSM) != 0)
	{	if  (strcmp(tab->tag,t)   != 0) {tab++; continue;}
		return tab;
	}
	return 0;
}
//==========================================================================================
//	Check for legible tag
//==========================================================================================
U_INT  GetOSMobjType(char *t ,char *v)
{	OSM_CONFP *cnf = LocateOSMvalue(t,v);
	return (cnf)?(cnf->otype):(0);	}

//==========================================================================================
//	Get tag value configuration
//==========================================================================================
void  GetOSMconfig(char *t ,char *v, OSM_CONFP &V)
{	//--- Skip if we already have major tag --------------------
	bool mt = (V.tag != 0) && (V.prop & OSM_PROP_MAJT);
	if (mt)		return;
	//--- Get this Tag -----------------------------------------
	OSM_CONFP *cnf = LocateOSMvalue(t,v);
	OSM_TAG   *tab = LocateOSMtag(t);
	V.otype = 0;
	if (0 == cnf) {STREETLOG("Tag (%s,%s) Skipped",t,v); return;}
	V	= *cnf;
	V.tag		= tab->tag;
	return;
}
//==========================================================================================
//	Set Property
//==========================================================================================
void SetOSMproperty(char *t, char *v, U_INT P)
{	OSM_CONFP *cnf = LocateOSMvalue(t,v);
	if (0 == cnf)			return;
	cnf->prop |= P;
	return;
}
//==========================================================================================
//	Return OSM directory
//==========================================================================================
char *GetOSMdirectory(U_INT otype)
{	if (otype >= OSM_MAX)	return "";
	int num  = replOBJ[otype];
	return     directoryTAB[num];
}
//==========================================================================================
//	Return OSM directory
//==========================================================================================
char GetOSMfolder(U_INT otype)
{	if (otype >= OSM_MAX) return 0;
	return replOBJ[otype];
}
//==========================================================================================
//	Return a replacement struct for the tag-value parameters 
//==========================================================================================
OSM_MDEF *GetOSMreplacement(char *T, char *V, char *obj)
{	U_INT otype =	GetOSMobjType(T, V);
	if (0 == otype)			return 0;
	//--- fill replacement parameters ------------------
	OSM_MDEF *rep = new OSM_MDEF();
	rep->otype	= otype;
	rep->dir		= GetOSMfolder(otype);
	rep->obj		= Dupplicate(obj,FNAM_MAX);
	return rep;
}
//==========================================================================
//  OSM Object
//	NOTE:		The first PART is for the main building
//					Additional parts may come from accessory like climbox, etc
//==========================================================================
OSM_Object::OSM_Object(CBuilder *B,OSM_CONFP *CF, D2_Style *sty)
{	bld					= B;
	type				= CF->otype;
	bvec				= CF->bvec;
	Layer				= CF->layr;
	State				= 1;
	bpm.stamp		= 0;
	bpm.obj			= this;
	part				= 0;
	orien       = 0;
	style				= 0;
	tag	= val		= 0;
	if (CF->tag)	tag = Dupplicate(CF->tag,64);
	if (CF->val)	val = Dupplicate(CF->val,64);
	if (sty)	ForceStyle(sty);
}
//-----------------------------------------------------------------
//	Destroy resources
//	Queue will be deleted when object is deleted and it will delete
//	all queued objects
//-----------------------------------------------------------------
OSM_Object::~OSM_Object()
{	int a = 0;
	if (tag)	delete tag;
	if (val)	delete val;
	RazPart();
}
//-----------------------------------------------------------------
//	Clear part
//-----------------------------------------------------------------
void OSM_Object::RazPart()
{	if (part)	 delete part;	
	part	= 0;
	return;
}
//-----------------------------------------------------------------
//	Transfer Queue and Clear items
//-----------------------------------------------------------------
void OSM_Object::Swap(Queue<D2_POINT> &H)
{	//--- Clear the receiver ---------------------------
	H.Clear();
	//--- Transfert base in foot print -----------------
	base.TransferQ(H);
	return;
}
//-----------------------------------------------------------------
//	Transfer Queue and invert items
//-----------------------------------------------------------------
void OSM_Object::Invert(Queue<D2_POINT> &H)
{	//--- Clear the receiver ---------------------------
	H.Clear();
	//--- Transfert base in foot print -----------------
	D2_POINT *pp;
	for (pp = base.Pop(); pp != 0; pp = base.Pop())
	{	H.PutHead(pp);	}
	RazPart();
	return;
}
//-----------------------------------------------------------------
//	Transfer queue
//-----------------------------------------------------------------
void OSM_Object::ReceiveBase(D2_POINT *p0)
{	D2_POINT *pp;
  D2_POINT *np;
	for (pp = p0; pp != 0; pp = pp->next)
	{	np = new D2_POINT(pp,'R');
		base.PutLast(np);
	}
	alti		= -bpm.geop.alt;
	return;
}
//-----------------------------------------------------------------
//	Rotation to align the building
//-----------------------------------------------------------------
U_CHAR OSM_Object::Rotate()
{	if (!CanBeRotated())	  return 0;
	double sn = sin(orien);
	double cn = cos(orien);
	part->ZRotation(sn,cn);
	return 1;
}
//-----------------------------------------------------------------
//	Change rotation
//-----------------------------------------------------------------
U_CHAR OSM_Object::IncOrientation(double rad)
{	orien = WrapTwoPi(orien + rad);
	double sn = sin(rad);
	double cn = cos(rad);
	part->ZRotation(sn,cn);
	return 1;
}
//-----------------------------------------------------------------
//	Edit Object
//-----------------------------------------------------------------
int OSM_Object::EditPrm(char *txt)
{	U_INT nb  = bpm.stamp;
	int   er  = bpm.error;
	double lx = FN_METRE_FROM_FEET(bpm.lgx);
	double ly = FN_METRE_FROM_FEET(bpm.lgy);
	switch (type)	{
		case OSM_LIGHT:
			_snprintf(txt,127,"Street Light (%d spots)",bpm.side);
			return er;
		case OSM_TREE:
			return er;
		default:
			_snprintf(txt,127,"BUILDING %05d lg:%.1lf wd:%.1lf",nb,lx,ly);
			}
	return er;
}
//-----------------------------------------------------------------
//	Edit Tag
//-----------------------------------------------------------------
void OSM_Object::EditTag(char *txt)
{	if (0 == tag)		return;
	_snprintf(txt,127,"%s : %s",tag,val);
	return;
}
//-----------------------------------------------------------------
//	Force Style parameters
//-----------------------------------------------------------------
void OSM_Object::ForceStyle(D2_Style *sty)
{	if (0 == sty)		return;
	sty->AssignBPM(&bpm);

	//--- search roof model and texture --------
	bpm.roofP			= sty->SelectedTexture();
	D2_Group *grp = sty->GetGroup();
	bpm.roofM     = grp->SelectedRoof();
	bpm.flNbr     = grp->GenFloorNbr();
}
//-----------------------------------------------------------------
//	Set texture and part
//-----------------------------------------------------------------
void OSM_Object::AssignBase()
{	part  = new C3DPart();
	TEXT_INFO     txd;
	txd.Dir		= FOLDER_OSM_USER;
	txd.apx   = 0xFF;
	txd.azp   = 0x00;
	strcpy(txd.name,"Base.jpg");
	//TRACE("GROUP LOAD TEXTURE %s",ntex);
	CShared3DTex *ref	= globals->txw->GetM3DPodTexture(txd);
	part->Reserve(ref);
	return;
}
//-----------------------------------------------------------------
//	Make a building
//-----------------------------------------------------------------
int OSM_Object::BuildBLDG(OSM_CONFP *C)
{	D2_Session *ses = bld->GetSession();
	bld->QualifyPoints(&bpm);
	//--- Set generation parameters ---------
	ses->GetaStyle(&bpm);
	AssignStyle(bpm.style,bld);
	//--------------------------------------
	return OSM_COMPLET;
}
//-----------------------------------------------------------------
//	Make a light row
//-----------------------------------------------------------------
int OSM_Object::BuildLITE(OSM_CONFP *CF)
{	D2_Session *ses = bld->GetSession();
	ReceiveBase(bld->FirstNode());
	BuildLightRow(6);						// 6 meters above ground
	ses->AddLight(this);
	bld->ClearNode();
	return OSM_COMPLET;
}
//-----------------------------------------------------------------
//	Make a forest 
//-----------------------------------------------------------------
int OSM_Object::BuildFRST(OSM_CONFP *CF)
{	D2_Session *ses = bld->GetSession();
	bld->QualifyPoints(&bpm);
	bld->Triangulation();
	ReceiveBase(bld->FirstNode());
	BuildForestTour(0);
	ses->AddForest(this);
	return OSM_PARTIAL;
}
//-----------------------------------------------------------------
//	Build a street lined with trees
//-----------------------------------------------------------------
int OSM_Object::BuildSTRT(OSM_CONFP *CF)
{	D2_Session *ses = bld->GetSession();
	OSM_MDEF   *mdf = ses->GetStreetTree();
	bld->QualifyPoints(&bpm);
	ReceiveBase(bld->FirstNode());
	BuildForestTour(mdf);
	ses->AddForest(this);
	return OSM_COMPLET;
	}
//-----------------------------------------------------------------
//	Build a ground object
//-----------------------------------------------------------------
int OSM_Object::BuildGRND(OSM_CONFP *C)
{	D2_Session *ses = bld->GetSession();
	bld->QualifyPoints(&bpm);
	ReceiveBase(bld->FirstNode());
	ses->AddGround(this);
	return OSM_COMPLET;
}
//-----------------------------------------------------------------
//	Build fortifications object
//-----------------------------------------------------------------
int OSM_Object::BuildWALL(OSM_CONFP *CF)
{	D2_Session *ses = bld->GetSession();
	bpm.flHtr				= FN_FEET_FROM_METER(CF->pm1);
	bld->QualifyPoints(&bpm);
	ReceiveBase(bld->FirstNode());
	bld->OrientFaces(&bpm);
	bld->BuildNormaFloor(0,0,0,&bpm);
	D2_Style	*sty	= ses->GetBaseStyle(CF->pm2);
	sty->AssignBPM(&bpm);
	bld->Texturing(&bpm);
	AssignBase();
	bld->SaveBuildingData(part,OSM_PROP_WALL);
	ses->AddForest(this);
	return OSM_COMPLET;
}
//-----------------------------------------------------------------
//	Assign style to object
//-----------------------------------------------------------------
void OSM_Object::AssignStyle(D2_Style *sty, CBuilder *B)
{ D2_Group *grp = sty->GetGroup();
	if (0 == bpm.flNbr)	grp->GenFloorNbr();
	//--- Get everything in bpm --------------------
	bpm.group	= grp;
	bpm.flNbr	= grp->GetFloorNbr();
	bpm.flHtr	= grp->GetFloorHtr();
	bpm.mans	= sty->IsMansart();
	if (0 == bpm.roofP)		bpm.roofP	= grp->Session()->GetRoofTexture(sty);
  if (0 == bpm.roofM)		grp->SelectOneRoof(&bpm);
	if (0 == bpm.roofM)		bpm.roofM	= grp->GetRoofModByNumber(bpm.mans);
	if (sty->HasHeight())	bpm.flHtr	= sty->GetHeight();
	bpm.zhtr	= bpm.flHtr;
	bpm.rtan	= B->GetTangent();
	//--- Add the building -------------------------
	grp->AddBuilding(this);
	bld->SetBDP(bpm);
	//--- Set part parameters ----------------------
	C3DPart      *prt  = new C3DPart();
	CShared3DTex *ref  = grp->GetTREF();
	prt->Reserve(ref);
	RazPart();
	part	= prt;
	B->RiseBuilding(&bpm);
	B->SaveBuildingData(prt,bpm.opt.GetAll());
	return;
}
//-----------------------------------------------------------------
//	Change style to object
//-----------------------------------------------------------------
void OSM_Object::ChangeStyle(D2_Style *sty, CBuilder *B)
{ //--- set block parameter to me----
	sty->AssignBPM(&bpm);
  bpm.opt.Raz(OSM_PROP_REPL);
	bpm.roofM	= 0;
	bpm.roofP	= 0;
	bpm.flNbr = 0;
	//--- Check for group change ------------
	D2_Group *grp = sty->GetGroup();
	if (bpm.group == grp)   return AssignStyle(sty,B);
	//--- Change group ----------------------
	D2_Group *pgp = bpm.group;
	pgp->RemBuilding(this);
	AssignStyle(sty,B);
	return;
}
//-----------------------------------------------------------------
//	Translate all vertices to SuperTile center
//-----------------------------------------------------------------
GN_VTAB *OSM_Object::StripToSupertile()
{ C_QGT *qgt			= globals->tcm->GetQGT(bpm.qgKey);
	CSuperTile *sup = qgt->GetSuperTile(bpm.supNo);
	SPosition   p0  = sup->GeoPosition();
	double rad      = FN_RAD_FROM_ARCS(p0.lat);
	double rdf      = cos(rad);
	SPosition   p1	= GetPosition();
	CVector T				= FeetComponents(p0, p1,rdf);
	//----------------------------------------------     
  U_INT nbv				= part->GetNBVTX();
	if (0 == nbv)		return 0;
	//--- Copy vertices from main part -------------
	GN_VTAB *src	= part->GetGTAB();
	GN_VTAB *vtx  = new GN_VTAB[nbv];
	GN_VTAB *dst  = vtx;
	for (U_INT k=0; k<nbv; k++)
	{	*dst	= *src++;
		 dst->Add(T);
		 dst++;
	}
	return vtx;
}
//-----------------------------------------------------------------
//	Return group texture reference
//-----------------------------------------------------------------
void *OSM_Object::GetGroupTREF()
{	if (0 == bpm.group)		return 0;
	return bpm.group->GetTREF();
}
//-----------------------------------------------------------------
//	Return Part texture reference
//-----------------------------------------------------------------
CShared3DTex *OSM_Object::GetPartTREF()
{return (part)?(part->GetTREF()):(0);}
//-----------------------------------------------------------------
//	Select this building
//-----------------------------------------------------------------
void OSM_Object::Select()
{	globals->osmS = this;
	bpm.selc	= 1;
	return;
}
//-----------------------------------------------------------------
//	Return Texture parameters
//-----------------------------------------------------------------
char *OSM_Object::TextureData(char &d)
{	CShared3DTex *ref = GetPartTREF();
	if (0 == ref)				return 0;
	return ref->TextureData(d);
}
//-----------------------------------------------------------------
//	Change selection on building
//-----------------------------------------------------------------
void OSM_Object::SwapSelect()
{	if (bpm.selc)	Deselect();
	else		Select();
	return;
}
//-----------------------------------------------------------------
//	Deselect this building
//-----------------------------------------------------------------
void OSM_Object::Deselect()
{	globals->osmS = 0;
	bpm.selc	= 0;
	return;
}
//-----------------------------------------------------------------
//	Replace  this object
//-----------------------------------------------------------------
void	OSM_Object::ReplaceBy(OSM_MDEF *rpp)
{	char *dir = directoryTAB[rpp->dir];
	repMD.Copy(*rpp);
  COBJparser fpar(OSM_OBJECT);
	fpar.SetDirectory(dir);
  fpar.Decode(repMD.obj,OSM_OBJECT);
	C3DPart *prt = fpar.BuildOSMPart(repMD.dir);
	if (0 == prt)							return;
	//--- Change model parameters --------------
	ReplacePart(prt);
	return; 
} 
//-----------------------------------------------------------------
//	Translate by adjust vector
//-----------------------------------------------------------------
void	OSM_Object::AdjustPart()
{	SVector T = bld->GetAdjustVector();
	if (part) part->Translate(T);
}
//-----------------------------------------------------------------
//	Change part. Object is replaced 
//-----------------------------------------------------------------
void OSM_Object::ReplacePart(C3DPart *prt)
{	orien	= atan2(repMD.sinA,repMD.cosA);
	bpm.opt.Set(OSM_PROP_REPL);
	if (part)	delete part;
	part = prt;
	Rotate();
	return;
}
//-----------------------------------------------------------------
//	Build a light row at the given height
//-----------------------------------------------------------------
void OSM_Object::BuildLightRow(double ht)
{	double H = FN_FEET_FROM_METER(ht);
	part		 = new C3DPart();
	part->AllocateOsmLIT(bpm.side);
	TEXT_INFO txd;
	strncpy(txd.name,"GLOBE.PNG",FNAM_MAX);
	txd.Dir = FOLDER_OSM_USER;
	CShared3DTex *ref = globals->txw->GetM3DPodTexture(txd);
	part->SetTREF(ref);
	//--- Init all vertices ----------------
	GN_VTAB *dst = part->GetGTAB();
	D2_POINT  *pps = 0;
	for (pps = base.GetFirst(); pps != 0; pps = pps->next)
	{	dst->VT_X = pps->x;
		dst->VT_Y	= pps->y;
		dst->VT_Z = pps->z + H;
		dst++;
	}
	return;
}

//-----------------------------------------------------------------
//	Set a tree at the spot
//	NOTE:  All trees must share the same texture ARBRES.tif
//-----------------------------------------------------------------
void OSM_Object::StoreTree(D2_POINT *pp, OSM_MDEF *md)
{	D2_Session *ses = bld->GetSession();	// call session
	OSM_MDEF   *mdf = (md)?(md):(ses->GetForestTree());				// Get a tree model
	char *dir				= directoryTAB[FOLDER_OSM_TREE];
	COBJparser fpar(OSM_OBJECT);
	fpar.SetDirectory(dir);
  fpar.Decode(mdf->obj,OSM_OBJECT);
	//--- Compute translation to the spot ------------
	double alfa = RandomNumber(180);
	double  rad = DegToRad(alfa);
	double   cs = cos(rad);
	double   sn = sin(rad);
	CVector T(pp->x,pp->y,pp->z);
	fpar.Transform(cs,sn,T);											// Translate vertices
	//--- Extend actual part with new vertices -------
	GN_VTAB *buf;
	int nbv = fpar.GetVerticeStrip(&buf);
	part->ExtendOSM(nbv,buf);
	delete buf;
	//TRACE("TREE at x=%lf y=%lf z%lf",pp->x,pp->y,pp->a);
	return;
}
//-----------------------------------------------------------------
//	Build a scan line about the forest width
//-----------------------------------------------------------------
void OSM_Object::ScanLine(double y)
{	double   x = minLon;
	D2_POINT pp;

	while (x < maxLon)
	{	pp.x  = RandomCentered(x,0,70);
		pp.y  = RandomCentered(y,0,70);
		x	   += bld->GetSession()->GetSeed();
		bool in = bld->PointInBase(&pp);
		if (!in)	continue;
		//--- Set a tree ----------------------
		CVector feet(pp.x,pp.y,0);
		SPosition pos    = bpm.geop;
		AddFeetTo(pos,feet);
		GroundSpot lnd(pos.lon,pos.lat);
		pp.a = globals->tcm->GetGroundAt(lnd);
		pp.z = pp.a - bpm.geop.alt;
		StoreTree(&pp,0);
	}
}
//-----------------------------------------------------------------
//	first step: seed a tree at each point
//	NOTE: Each tree is a 4 triangles figure, thus 
//				Each tree has 12 vertices
//-----------------------------------------------------------------
void OSM_Object::BuildForestTour(OSM_MDEF *mdf)
{	minLat = minLon = 0;
	maxLat = maxLon	= 0;
	part		 = new C3DPart();
	TEXT_INFO txd;
	strncpy(txd.name,"ARBRES.TIF",FNAM_MAX);
	txd.Dir = FOLDER_OSM_TREE;
	CShared3DTex *ref = globals->txw->GetM3DPodTexture(txd);
	part->SetTREF(ref);
	//-----------------------------------------------------
	for (D2_POINT *pp=base.GetFirst(); pp != 0; pp = pp->next)
	{	StoreTree(pp, mdf);
		if (pp->y < minLat)	minLat = pp->y;
		if (pp->y > maxLat)	maxLat = pp->y;
		if (pp->x < minLon) minLon = pp->x;
		if (pp->x > maxLon)	maxLon = pp->x;
	}
	pm1	= minLat;
	return;
	}
//-----------------------------------------------------------------
//	Make a forest row
//-----------------------------------------------------------------
int OSM_Object::BuildROWF(OSM_CONFP *CF)
{ //--- Check for surface -------------------------------
	if (bpm.surf < SQRF_FROM_SQRMTR(100))	return OSM_COMPLET;	
  ScanLine(pm1);
	pm1 += bld->GetSession()->GetSeed();
	return (pm1 < maxLat)?(OSM_PARTIAL):(OSM_COMPLET);
}
//-----------------------------------------------------------------
//	Draw next row
//-----------------------------------------------------------------
int OSM_Object::NextForestRow()
{	//--- Check for surface -------------------------------
	if (bpm.surf < SQRF_FROM_SQRMTR(100))	return OSM_COMPLET;
	ScanLine(pm1);												// Build a row
	pm1 += 90;
	return (pm1 > maxLat)?(OSM_COMPLET):(OSM_PARTIAL);
}
//-----------------------------------------------------------------
//	Draw as a terrain object
//-----------------------------------------------------------------
void OSM_Object::Draw()
{	//--- check for selected object -----------------
  bool ok		= (this != globals->osmS) || globals->clk->GetON();
			 ok  &= (State == 1);
	if (!ok)		return;
	//--- Get line mode -----------------------------
	U_INT mode = bld->DrawMode();
	bool  tour = (mode == 0) && (bpm.selc);
	//--- Draw my parts -----------------------------
	glLoadName(bpm.stamp);
	glPushMatrix();
	CVector T = FeetComponents(globals->geop, this->bpm.geop, bpm.rdf);
	glTranslated(T.x, T.y, T.z);  //T.z);
	if (part)	part->Draw();
	if (tour)	DrawBase();	
	glPopMatrix();
	return;
}
//-----------------------------------------------------------------
//	Draw as a Building object
//	We dont translate camera at building center
//-----------------------------------------------------------------
void OSM_Object::DrawAsBLDG()
{	DebDrawOSMbuilding();	
	part->Draw();
	EndDrawOSM();
	
}
//-----------------------------------------------------------------
//	Draw as a ground object
//	We dont translate camera at building center
//-----------------------------------------------------------------
void OSM_Object::DrawAsGRND()
{	DebDrawOSMground();
	DrawBase();
	EndDrawOSM();
}
//-----------------------------------------------------------------
//	Draw as a double side object
//	We dont translate camera at building center
//-----------------------------------------------------------------
void OSM_Object::DrawAsDBLE()
{	DebDrawOSMground();
	part->Draw();
	EndDrawOSM();
}
//-----------------------------------------------------------------
//	Draw Tour
//-----------------------------------------------------------------
void OSM_Object::DrawBase()
{ glColor4f(1,1,1,1);
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINE_LOOP);
	for (D2_POINT *pp=base.GetFirst(); pp != 0; pp = pp->next)
	{	double htr = (NeedZ())?(pp->a):(0);
		//glVertex3d(pp->x,pp->y,htr);
		glVertex3d(pp->x,pp->y,pp->z);	
		}
	glEnd();
	glEnable(GL_TEXTURE_2D);
	return;
}
//-----------------------------------------------------------------
//	Draw as a Tree object
//	We dont translate camera at building center
//-----------------------------------------------------------------
void OSM_Object::DrawAsTREE()
{	U_INT mode = bld->DrawMode();
	DebDrawOSMforest();
	part->Draw();
	if (0 == mode) bld->DrawGround(1);
	EndDrawOSM();
	return;
}
//-----------------------------------------------------------------
//	Draw as a light row
//-----------------------------------------------------------------
void OSM_Object::DrawAsLITE()
{	DebDrawOSMlight(lightOSM, alphaOSM);
	part->Draw();
  EndDrawOSM();
}
//-----------------------------------------------------------------
//	Write the building
//-----------------------------------------------------------------
void OSM_Object::WriteAsBLDG(FILE *fp)
{	char txt[128];
	_snprintf(txt,127,"Start %05d id=%d\n", bpm.stamp, bpm.ident);
	fputs(txt,fp);
	//--- Write style ----------------------------------
	char *nsty = bpm.style->GetSlotName();
	int   rmno = bpm.roofM->GetRoofModNumber();
	int   rofn = bpm.roofP->GetRoofTexNumber();
	int   flnb = bpm.flNbr;
	_snprintf(txt,127,"Style %s rofm=%d rftx=%d flNbr=%d \n",nsty,rmno,rofn,flnb);
	fputs(txt,fp);
	//--- Write tag ------------------------------------
	char *vl = (tag)?(val):("---");
	if (tag)
		{	_snprintf(txt,127,"Tag (%s = %s)\n",tag,vl);
			fputs(txt,fp);
		}
	//--- Write all points -----------------------------
	D2_POINT *pp;
	for (pp = base.GetFirst(); pp != 0; pp = pp->next)
	{	_snprintf(txt,127,"V(%.7lf, %.7lf)\n", pp->dgy, pp->dgx);
		fputs(txt,fp);
	}
	//--- Write replace if any -------------------------
	if (0 == repMD.obj)			return;
	//--- Write the replace directive ------------------
	_snprintf(txt,127,"Replace (Z=%.7lf) with %s\n",orien,repMD.obj);
	fputs(txt,fp);
	//--- all ok ---------------------------------------
	return;
}
//-----------------------------------------------------------------
//	Write as a generic object
//-----------------------------------------------------------------
void OSM_Object::WriteAsGOSM(FILE *fp)
{	char txt[128];
	_snprintf(txt,127,"Start %05d id=%d\n", bpm.stamp, bpm.ident);
	fputs(txt,fp);
	//--- Write tag ------------------------------------
	char *vl = (tag)?(val):("---");
	if (tag)
		{	_snprintf(txt,127,"Tag (%s = %s)\n",tag,vl);
			fputs(txt,fp);
		}
	//--- Write all points -----------------------------
	D2_POINT *pp;
	for (pp = base.GetFirst(); pp != 0; pp = pp->next)
	{	_snprintf(txt,127,"V(%.7lf, %.7lf)\n", pp->dgy, pp->dgx);
		fputs(txt,fp);
	}
	//--- all ok ---------------------------------------
	return;
}
//-----------------------------------------------------------------
//	Skip write
//-----------------------------------------------------------------
void OSM_Object::SkipWrite(FILE *fp)
{	return;	}
//==================================================================
//	Draw as a local object
//==================================================================
void OSM_Object::DrawLocal()
{	if (0 == State)		return;
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	(this->*renderOSM[Layer])();
	glPopAttrib();
	return;
}
//==================================================================
//	Write object
//==================================================================
void OSM_Object::Write(FILE *p)
{	if (0 == State)		return;
	(this->*writeOSM[bvec])(p);
	return;
}

//====================== END OF FILE ============================================
