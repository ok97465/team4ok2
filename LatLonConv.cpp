//---------------------------------------------------------------------------
#include "LatLonConv.h"

//---------------------------------------------------------------------------

#define EPS 0.00000000000005          // a small number
//---------------------------------------------------------------------------
TCoordConvStatus VInverse(double Latitude1, double Longitude1,
						  double Latitude2, double Longitude2,
						  double *Distance,   double *Azimuth12,
						  double *Azimuth21)
{
	// Check for antipodal points
	if (IsAntipodal(Latitude1, Latitude2, Longitude1, Longitude2)) return(ANTIPODAL);

	// Check for same point
	if ((fabs(Latitude1 - Latitude2) < 1e-6) && (fabs(Longitude1 - Longitude2) < 1e-6))
	{
		*Azimuth12 = 0.0;
		*Azimuth21 = 0.0;
		*Distance = 0.0;
		return(SAMEPT);
	}

	const double a0 = EllipseMajor;
	const double b0 = EllipseMinor;
	const double flat = (a0 - b0) / a0;
	const double r = 1.0 - flat;

	const double lat1 = DEGTORAD(Latitude1);
	const double lat2 = DEGTORAD(Latitude2);
	const double long1 = DEGTORAD(Longitude1);
	const double long2 = DEGTORAD(Longitude2);

	const double tanu1 = r * tan(lat1);
	const double cosu1 = 1.0 / sqrt(1.0 + tanu1 * tanu1);
	const double sinu1 = tanu1 * cosu1;

	const double tanu2 = r * tan(lat2);
	const double cosu2 = 1.0 / sqrt(1.0 + tanu2 * tanu2);
	const double sinu2 = tanu2 * cosu2;

	const double L = long2 - long1;
	double lambda = L;
	double testlambda;

	double sigma, ss, cs, sinalpha, cosalpha_sqr, c2sm, C;

	int icount = 0;
	const int MAXCT = 100;

	do
	{
		icount++;
		testlambda = lambda;

		const double sin_lambda = sin(lambda);
		const double cos_lambda = cos(lambda);

		// Equation 14
		const double s2s_sq = pow(cosu2 * sin_lambda, 2.0) + pow(cosu1 * sinu2 - sinu1 * cosu2 * cos_lambda, 2.0);
		ss = sqrt(s2s_sq);
		// Equation 15
		cs = sinu1 * sinu2 + cosu1 * cosu2 * cos_lambda;

		sigma = atan2(ss, cs);
		// Equation 17
		sinalpha = cosu1 * cosu2 * sin_lambda / ss;
		cosalpha_sqr = 1.0 - sinalpha * sinalpha;
		// Equation 18
		c2sm = cs - (2.0 * sinu1 * sinu2 / cosalpha_sqr);
		// Equation 10
		C = flat / 16.0 * cosalpha_sqr * (4.0 + flat * (4.0 - 3.0 * cosalpha_sqr));

		// Equation 11
		lambda = L + (1.0 - C) * flat * sinalpha * (sigma + C * ss * (c2sm + C * cs * (-1.0 + 2.0 * c2sm * c2sm)));
	}
	while ((fabs(testlambda - lambda) > EPS) && (icount <= MAXCT));

	if (icount > MAXCT) return(NOCONVERGE);

	const double u2 = cosalpha_sqr * (a0 * a0 - b0 * b0) / (b0 * b0);

	const double A = 1.0 + (u2 / 16384.0) * (4096.0 + u2 * (-768.0 + u2 * (320.0 - 175.0 * u2)));
	const double B = (u2 / 1024.0) * (256.0 + u2 * (-128.0 + u2 * (74.0 - 47.0 * u2)));

	const double dsigma = B * ss * (c2sm + (B / 4.0) * (cs * (-1.0 + 2.0 * c2sm * c2sm) -
		(B / 6.0) * c2sm * (-3.0 + 4.0 * ss * ss) * (-3.0 + 4.0 * c2sm * c2sm)));

	const double s = b0 * A * (sigma - dsigma);

	const double sin_lambda_final = sin(lambda);
	const double cos_lambda_final = cos(lambda);

	double alpha12 = atan2(cosu2 * sin_lambda_final, (cosu1 * sinu2 - sinu1 * cosu2 * cos_lambda_final));
	double alpha21 = atan2(cosu1 * sin_lambda_final, (-sinu1 * cosu2 + cosu1 * sinu2 * cos_lambda_final));

	*Azimuth12 = RADTODEG(ModAzimuth(alpha12));
	*Azimuth21 = RADTODEG(ModAzimuth(alpha21 - M_PI));
	*Distance = s;

	return OKNOERROR;
}
//---------------------------------------------------------------------------
TCoordConvStatus VDirect(double Latitude1,  double Longitude1,
						  double Azimuth12,  double Distance,
						  double *Latitude2, double *Longitude2,
						  double *Azimuth21)
{
	// Handle zero distance case first
	if (Distance == 0.0)
	{
		*Latitude2 = Latitude1;
		*Longitude2 = Longitude1;
		*Azimuth21 = 0.0;
		return(ZERODIST);
	}

	const double s = Distance;
	const double lat1 = DEGTORAD(Latitude1);
	const double long1 = DEGTORAD(Longitude1);
	const double az12 = DEGTORAD(Azimuth12);

	const double a0 = EllipseMajor;
	const double b0 = EllipseMinor;
	const double flat = (a0 - b0) / a0;
	const double r = 1.0 - flat;

	const double tanu1 = r * tan(lat1);
	const double u1 = atan(tanu1);
	const double sinu1 = sin(u1);
	const double cosu1 = cos(u1);

	const double sin_az12 = sin(az12);
	const double cos_az12 = cos(az12);

	const double sinalpha = cosu1 * sin_az12;
	const double cosalpha_sqr = 1.0 - sinalpha * sinalpha;
	const double cosalpha = sqrt(cosalpha_sqr);

	const double usqr = cosalpha_sqr * (a0 * a0 - b0 * b0) / (b0 * b0);

	const double A = 1.0 + usqr / 16384.0 * (4096.0 + usqr * (-768.0 + usqr * (320.0 - 175.0 * usqr)));
	const double B = usqr / 1024.0 * (256.0 + usqr * (-128.0 + usqr * (74.0 - 47.0 * usqr)));

	const double sigma1 = atan2(tanu1, cos_az12);
	double sigma = s / (b0 * A);
	double lastsigma;
	double sin_sigma, cos_sigma, cos2sigmam;

	do
	{
		lastsigma = sigma;
		const double twosigmam = 2.0 * sigma1 + sigma;
		sin_sigma = sin(sigma);
		cos_sigma = cos(sigma);
		cos2sigmam = cos(twosigmam);

		const double deltasigma = B * sin_sigma * (cos2sigmam + B / 4.0 * (cos_sigma * (-1.0 + 2.0 * cos2sigmam * cos2sigmam) -
			B / 6.0 * cos2sigmam * (-3.0 + 4.0 * sin_sigma * sin_sigma) * (-3.0 + 4.0 * cos2sigmam * cos2sigmam)));

		sigma = s / (b0 * A) + deltasigma;
	}
	while (fabs(sigma - lastsigma) > EPS);

	// Recalculate final values with converged sigma
	const double twosigmam = 2.0 * sigma1 + sigma;
	sin_sigma = sin(sigma);
	cos_sigma = cos(sigma);
	cos2sigmam = cos(twosigmam);

	// Final Latitude
	const double lat2_num = sinu1 * cos_sigma + cosu1 * sin_sigma * cos_az12;
	const double lat2_den = r * sqrt(sinalpha * sinalpha + pow(sinu1 * sin_sigma - cosu1 * cos_sigma * cos_az12, 2.0));
	double lat2 = atan2(lat2_num, lat2_den);

	// Final Longitude
	const double lambda_num = sin_sigma * sin_az12;
	const double lambda_den = cosu1 * cos_sigma - sinu1 * sin_sigma * cos_az12;
	const double lambda = atan2(lambda_num, lambda_den);

	const double C = flat / 16.0 * cosalpha_sqr * (4.0 + flat * (4.0 - 3.0 * cosalpha_sqr));
	const double omega = lambda - (1.0 - C) * flat * sinalpha *
		(sigma + C * sin_sigma * (cos2sigmam + C * cos_sigma * (-1.0 + 2.0 * cos2sigmam * cos2sigmam)));
	double long2 = long1 + omega;

	// Final Azimuth
	const double az21_den = -sinu1 * sin_sigma + cosu1 * cos_sigma * cos_az12;
	double az21 = atan2(sinalpha, az21_den);

	// Normalize and convert to degrees
	lat2 = ModLatitude(lat2);
	long2 = ModLongitude(long2);
	az21 = ModAzimuth(az21);

	*Latitude2 = RADTODEG(lat2);
	*Longitude2 = RADTODEG(long2);
	*Azimuth21 = RADTODEG(az21);

	return(OKNOERROR);
}
//---------------------------------------------------------------------------
 static bool IsAntipodal(double Latitude1, double Latitude2,
						 double Longitude1,double Longitude2)
 {
   double la,lo;
   int eflag;

   eflag = Antipod(Latitude1, Longitude1, &la, &lo);  //compute antipodal values
  if (eflag != OKNOERROR)  return(false);

  if ((Latitude2 == la) && (Longitude2 == lo)) return(true);
  else return(false);
 }
//---------------------------------------------------------------------------
 static TCoordConvStatus Antipod(double latin,   double lonin,
								 double *latout, double *lonout)
{
  //this function returns the antipod of latin, lonin in latout, lonout
  //NOTE: angles are assumed to be in decimal degrees

  //function returns:
  //   OkNOERROR if successful and latout and lonouut contain the
  //       antipodal values otherwise they are indeterminate. I set them
  //       both to 9999.0 degrees.
  //   LONGERR if abs(lonin) is > 180 degrees
  //   LATERR if abs(latin) is > 90 degrees


  if (fabs(lonin) > 180.0)
	{
	  *latout = 9999.0;
	  *lonout = 9999.0;
	  return(LONGERR);
	}

  if (fabs(latin) > 90.0)
	{
	  *latout = 9999.0;
	  *lonout = 9999.0;
	  return(LATERR);

	}

  *latout = -latin;
  *lonout= modulus(lonin + 180.0, 180.0);
  return(OKNOERROR);

}
//---------------------------------------------------------------------------
#if 0
static double modulus (double const X, double Y)
{
 double Z,Result;

  Result = X / Y;
  Z = (int)Result;
  if (Frac(Result) < 0.0) Z = Z - 1.0;
  Result = X - Y * Z;
  return(Result);
}
#endif
//---------------------------------------------------------------------------
static double modulus(double Num1, double Num2)
{
  return(Num1 - Num2 * floor(Num1 / Num2));
}
//---------------------------------------------------------------------------
static double Frac(double Num1)
{
 return(Num1-((int)Num1));
}
//---------------------------------------------------------------------------
static double sqr(double X)
{
 return(X*X);
}
//---------------------------------------------------------------------------
static double ModAzimuth(double az)
{
 return(modulus(az, 2.0 * M_PI));
}
//---------------------------------------------------------------------------
static double ModLatitude(double lat)
{
 return(modulus(lat + M_PI / 2.0, M_PI) - M_PI / 2.0);
}
//---------------------------------------------------------------------------
static double ModLongitude(double lon)
{
 return(modulus(lon + M_PI, 2.0 * M_PI) - M_PI);
}
//---------------------------------------------------------------------------

