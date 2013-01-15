////////////////////////////////////////////////////////////////////////////////

#include "quadrature.h"

////////////////////////////////////////////////////////////////////////////////

static double gauss_location[10][10] = {
	{0,0,0,0,0,0,0,0,0,0},
	{-0.5773502691896257,+0.5773502691896257,0,0,0,0,0,0,0,0},
	{-0.7745966692414835,-0.0000000000000000,+0.7745966692414833,0,0,0,0,0,0,0},
	{-0.8611363115940520,-0.3399810435848561,+0.3399810435848563,+0.8611363115940521,0,0,0,0,0,0},
	{-0.9061798459386636,-0.5384693101056829,+0.0000000000000000,+0.5384693101056832,+0.9061798459386634,0,0,0,0,0},
	{-0.9324695142031526,-0.6612093864662647,-0.2386191860831970,+0.2386191860831969,+0.6612093864662645,+0.9324695142031518,0,0,0,0},
	{-0.9491079123427589,-0.7415311855993947,-0.4058451513773977,+0.0000000000000000,+0.4058451513773971,+0.7415311855993949,+0.9491079123427583,0,0,0},
	{-0.9602898564975368,-0.7966664774136265,-0.5255324099163288,-0.1834346424956501,+0.1834346424956497,+0.5255324099163289,+0.7966664774136263,+0.9602898564975370,0,0},
	{-0.9681602395076256,-0.8360311073266358,-0.6133714327005899,-0.3242534234038091,+0.0000000000000000,+0.3242534234038089,+0.6133714327005904,+0.8360311073266354,+0.9681602395076258,0},
	{-0.9739065285171719,-0.8650633666889838,-0.6794095682990255,-0.4333953941292479,-0.1488743389816311,+0.1488743389816314,+0.4333953941292472,+0.6794095682990247,+0.8650633666889845,+0.9739065285171725}};

static double gauss_weight[10][10] = {
	{2.0000000000000000,0,0,0,0,0,0,0,0,0},
	{0.9999999999999998,0.9999999999999998,0,0,0,0,0,0,0,0},
	{0.5555555555555556,0.8888888888888894,0.5555555555555549,0,0,0,0,0,0,0},
	{0.3478548451374539,0.6521451548625458,0.6521451548625463,0.3478548451374539,0,0,0,0,0,0},
	{0.2369268850561884,0.4786286704993670,0.5688888888888881,0.4786286704993667,0.2369268850561888,0,0,0,0,0},
	{0.1713244923791704,0.3607615730481389,0.4679139345726915,0.4679139345726909,0.3607615730481390,0.1713244923791699,0,0,0,0},
	{0.1294849661688695,0.2797053914892769,0.3818300505051178,0.4179591836734693,0.3818300505051188,0.2797053914892766,0.1294849661688693,0,0,0},
	{0.1012285362903762,0.2223810344533752,0.3137066458778861,0.3626837833783620,0.3626837833783624,0.3137066458778872,0.2223810344533738,0.1012285362903761,0,0},
	{0.0812743883615750,0.1806481606948578,0.2606106964029348,0.3123470770400034,0.3302393550012596,0.3123470770400030,0.2606106964029359,0.1806481606948576,0.0812743883615739,0},
	{0.0666713443086876,0.1494513491505793,0.2190863625159838,0.2692667193099970,0.2955242247147519,0.2955242247147532,0.2692667193099968,0.2190863625159814,0.1494513491505807,0.0666713443086876}};


static double hammer_location[12][2][12] = {
	{{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0}},
	{{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0}},
	{{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0}},
	{{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0}},
	{{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0}},
	{{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0}},
	{{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0}},
	{{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0}},
	{{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0}},
	{{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0}},
	{{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0}},
	{{0.0630890144915020,0.8738219710169960,0.0630890144915020,0.2492867451709100,0.5014265096581790,0.2492867451709100,0.3103524510337850,0.0531450498448160,0.6365024991213990,0.3103524510337850,0.0531450498448160,0.6365024991213990},{0.0630890144915020,0.0630890144915020,0.8738219710169960,0.2492867451709100,0.2492867451709100,0.5014265096581790,0.0531450498448160,0.3103524510337850,0.3103524510337850,0.6365024991213990,0.6365024991213990,0.0531450498448160}}};

static double hammer_weight[12][12] = {
	{0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0},
	{0.0254224531851035,0.0254224531851035,0.0254224531851035,0.0583931378631895,0.0583931378631895,0.0583931378631895,0.0414255378091870,0.0414255378091870,0.0414255378091870,0.0414255378091870,0.0414255378091870,0.0414255378091870}};

////////////////////////////////////////////////////////////////////////////////

double quadrature_gauss_location(int order, int index)
{
	return gauss_location[order][index];
}

double quadrature_gauss_weight(int order, int index)
{
	return gauss_weight[order][index];
}

double quadrature_hammer_location(int order, int dimension, int index)
{
	return hammer_location[order][dimension][index];
}

double quadrature_hammer_weight(int order, int index)
{
	return hammer_weight[order][index];
}

////////////////////////////////////////////////////////////////////////////////