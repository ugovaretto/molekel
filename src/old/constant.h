#ifndef _CONSTANT_H_
#define _CONSTANT_H_
//
// Molekel - Molecular Visualization Program
// Copyright (C) 2006, 2007, 2008, 2009 Swiss National Supercomputing Centre (CSCS)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301, USA.
//
// $Author$
// $Date$
// $Revision$
//

// Previous copyright notice from molekel 4.6
//
///*  MOLEKEL, Version 4.4, Date: 10.Dec.03
// *  Copyright (C) 2002-2003 Claudio Redaelli (CSCS)
// *  (original IRIX GL implementation, concept and data structure
// *   by Peter F. Fluekiger, CSCS/UNI Geneva, OpenGL/Mesa extensions
// *   and revisions by Stefan Portmann, CSCS/ETHZ)
// */


#include "molekeltypes.h"

#if defined(TRANSPARENT)
#undef TRANSPARENT
#endif

#if defined(COLOR_MENU)
#undef COLOR_MENU
#endif

#define BOHR         0.529177249
#define _1_BOHR      1.88972599

#define NLIST         105

#define ROT                1001
#define TRANS              1002
#define SCALE              1003
#define RESET              1004
#define SPIN               1005
#define PERSP              1006
#define BACK               1007
#define NO_H               1008
#define BOX                1009
#define CENTER             1010
#define WIRE               1011
#define LABELS             1012
#define TRANSPARENT        1013
#define SURF_CLIP          1014
#define CHICKENWIRE        1015
#define FULLSCREEN         1016
#define ATM_CHAR           1017
#define ATM_SPIN           1018
#define TOGGLE_MULT        1019
#define DEL_BOND           1020
#define ADD_BOND           1021
#define SWITCH_MOL         1022
#define DEPTHCUE           1023
#define MANIP_ALL          1024
#define ZOOMBOX            1025
#define RESETBOX           1026
#define ACTIV_LOGO         1027
#define INFO               1028
#define LOAD_MACU          1029
#define CUBEPLANES         1030
#define PROJECT_MACU       1031
#define FREE_PROJECTION    1032
#define ADD_MACU           1033
#define SUBST_MACU         1034
#define PICK_ATOM          1035
#define PICK_BOND          1036
#define PICK_BOX           1037
#define MOVEPLANE          1038
#define PICK_SURF          1039
#define PICK_TRI           1040
#define CONNECT            1041
#define LOAD_DOTS          1042
#define LOAD_SLD           1043
#define WRITE_PDB          1044
#define WRITE_SLD          1045
#define WRITE_MS           1046
#define LOAD_GAUSS         1047
#define CALC_ORB           1048
#define EL_DENS            1049
#define SPIN_DENS          1050
#define ATOM_SPIN          1051
#define VIBRATION          1052
#define MEP                1053
#define ADD_SURF           1054
#define DEL_SURF           1055
#define GAUSS_ORB          1056
#define GAMESS_ORB         1057
#define LOAD_MKL           1058
#define DISTANCE           1059
#define VALENCE            1060
#define DIHEDRAL           1061
#define ATOMDATA           1062
#define MATCH              1063
#define ADD_DUMMY          1064
#define REM_DUMMY          1065
#define ALLDIST            1066
#define BOND_COL           1067
#define SINGLEBOND         1068
#define DOUBLEBOND         1069
#define TRIPLEBOND         1070
#define FLATSHADE          1071
#define CONNOLLY           1072
#define EHT_ORB            1073
#define SURFCOLOR          1074
#define ATOMCOLOR          1075
#define BONDCOLOR          1076
#define BACKCOLOR1         1077
#define BACKCOLOR2         1078
#define LABELCOLOR         1079
#define ARROWCOLOR         1080
#define BOXCOLOR           1081
#define DEL_MOL            1082
#define ADF_ORB            1083
#define PRDDO_ORB          1084
#define PICK_SEGMENT       1085
#define LOAD_ADF           1086
#define TEXTURE            1087
#define TEXTURE_UP         1088
#define TEXTURE_DOWN       1089
#define LOAD_TEXTURE       1090
#define SURF_TEXTURE       1091
#define BOND_TEXTURE       1092
#define ATOM_TEXTURE       1093
#define BACK_TEXTURE       1094
#define SURF_REFLECT       1095
#define BOND_REFLECT       1096
#define ATOM_REFLECT       1097
#define SURF_PHONG         1098
#define TEX_MAP            1099
#define TEX_REFLECT        1100
#define TEX_PHONG          1101
#define SHADOW             1102
#define ACBUF              1103
#define RESIDUES           1104
#define AMOSS_ORB          1105
#define LOAD_HONDO         1106
#define HONDO_ORB          1107
#define LOAD_MOS           1108
#define MOS_ORB            1109
#define ZINDO_ORB          1110
#define LOAD_ZINDO         1111
#define ADF_ORB_A          1112
#define ADF_ORB_B          1113
#define COLOR_MENU         1114
#define SURF_MENU          1115
#define TEXTURE_MENU       1116
#define QUIT               1117
#define LOAD_PDB           1118
#define MOLMOD             1119
#define BOND_ATTR          1120
#define HELP               1121
#define OPTION             1122
#define OK                 1123
#define CANCEL             1124
#define USE_COEFFS         1125
#define USE_MATRICES       1126
#define YES                1127
#define NO                 1128
#define FREQ_ARROW         1129
#define DUMMY              1130
#define CANCEL_PLANE       1131
#define XYZ_PLANE          1132
#define UNLOAD_MACU        1133
#define POP_MAININTERF     1134
#define GRID_VALUES        1135
#define FAST_SURF          1136
#define DONE               1137
#define REM_VALUES         1138
#define TEXTSTR            1139
#define LOAD_GCUBE         1140
#define PICK_MOL           1141
#define SWITCH_SURF        1142
#define DOTSLD             1143
#define CANCEL_DOTSLD      1144
#define SURF_OPT           1145
#define CANCEL_SURF_OPT    1146
#define CANCEL_OPT         1147
#define VDW_SCALE          1148
#define BOND_THICK         1149
#define FREQ               1150
#define STOP               1151
#define FREQ_SCALE         1152
#define SOLID_ARROW        1153
#define PLAY               1154
#define PLAY_F             1155
#define PLAY_B             1156
#define PLAY_FIRST         1157
#define PLAY_LAST          1158
#define NEXT_F             1159
#define NEXT_B             1160
#define BOXVAL_UPDATE      1161
#define WRITE_PDB_ORIG     1162
#define WRITE_PDB_CURRENT  1163
#define WRITE_RGB          1164
#define WRITE_TIFF         1165
#define WRITE_TIFF_LZW     1166
#define WRITE_TIFF_PACK    1167
#define LOAD_MULTI_PDB     1168
#define LOAD_XYZ           1169
#define WRITE_XYZ_ORIG     1170
#define WRITE_XYZ_CURRENT  1171
#define LOAD_GAMESS        1172
#define LOAD_HONDO_PUNCH   1173
#define LOAD_T21           1174
#define DEFCOLOR           1175
#define LOAD_T41           1176
#define DISTCOLOR          1177
#define ANGELCOLOR         1178
#define DIHEDCOLOR         1179
#define ALPHABLENDING      1180
#define GET_ATOMCOLOR      1181
#define GET_BONDCOLOR      1182
#define GET_SURFCOLOR      1183
#define GET_ARROWCOLOR     1184
#define GET_BOXCOLOR       1185
#define DUPLEX_TEXTURE     1186
#define LOAD_PRDDO         1187
#define LOAD_MULTI_XYZ     1188
#define WARRANTY           1189
#define LICENSE            1190
#define TGL_IDLE           1191
#define LOAD_NBO_ORB       1192
#define RENDER             1193
#define RGB_CURR_IMG       1194
#define RGB_FREQ_IMG       1195
#define RGB_STRC_IMG       1196
#define TIF_CURR_IMG       1197
#define TIF_FREQ_IMG       1198
#define TIF_STRC_IMG       1199
#define TIFLZW_CURR_IMG    1200
#define TIFLZW_FREQ_IMG    1201
#define TIFLZW_STRC_IMG    1202
#define TIFPACK_CURR_IMG   1203
#define TIFPACK_FREQ_IMG   1204
#define TIFPACK_STRC_IMG   1205
#define PPM_CURR_IMG       1206
#define PPM_FREQ_IMG       1207
#define PPM_STRC_IMG       1208
#define ADJUST_IMG_W       1209
#define ADJUST_IMG_H       1210
#define SET_WSIZE          1211
#define EN_DIS_ABLE        1212
#define PPM                1213
#define WRITE_JPEG         1214
#define JPEG_CURR_IMG      1215
#define JPEG_FREQ_IMG      1216
#define JPEG_STRC_IMG      1217
#define DIPOLE             1218
#define SHOW_DIPOLE        1219
#define DIPOLE_SCALE       1220
#define SET_CUTPLANE       1221
#define H_BOND             1222
#define SHOW_H_BOND        1223
#define UPDATE_H_BOND      1224
#define LOAD_MOLDEN        1225
#define LOAD_MOLDEN_FREQ   1226
#define LOAD_MOLDEN_GEOM   1227
#define MLD_SLATER_ORB     1228
#define LOAD_DOTVAL        1229
#define WRITE_DOTVAL       1230

#endif
