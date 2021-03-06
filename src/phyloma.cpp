/*
 * growth rate of log ne (t) ~ BM 
 * 
 */

// [[Rcpp::depends(RcppArmadillo)]]

#include <R.h>
#include <RcppArmadillo.h>

using namespace Rcpp; 
using namespace arma; 
using namespace std; 

/* 	
   .of1 <- function(  logne , xtau = tau0)
	{
		ne <- exp(logne)
		ll <- sum( with(tredat, dpois( nco , dh*(ltt * (ltt-1.)/2.)/ ne, log=T))  )
		grs <- diff ( rev(logne) ) / ( rev(logne[-1]) ) / dh #
		ll <- ll + sum( dnorm( diff( grs ), 0, sqrt( dh / xtau ) , log = TRUE) )  + tau_logprior( xtau )
		ll
	}
*/

//~ double rbm_loglik1( vec logne, double tau,  double dh ) {
	//~ vec difflogne = arma::diff ( logne )  ;
	//~ vec grs = (difflogne / logne.head( difflogne.size() ) )/dh; 
	//~ vec diffgrs = arma::diff( grs ); 
	//~ double xsd = sqrt( dh / tau ); 
	//~ double ll = 0.; 
	//~ for (int i = 0; i < diffgrs.size(); i++){
		//~ ll += R::dnorm( diffgrs(i), 0., xsd, 1 );
	//~ }
	//~ return ll; 
//~ }

double rbm_loglik1_2( arma::vec ne, double tau,  double dh ) {
	arma::vec diffne = arma::diff ( ne )  ;
	arma::vec grs = (diffne / ne.head( diffne.size() ) )/dh; 
	arma::vec diffgrs = arma::diff( grs ); 
	double xsd = sqrt( dh / tau ); 
	double ll = 0.; 
	for (int i = 0; i < diffgrs.size(); i++){
		ll += R::dnorm( diffgrs(i), 0., xsd, 1 );
	}
	return ll; 
}

// includes covars 
double rbm_loglik2_2( arma::vec ne, double tau, double dh, arma::vec diff_zxb  ){
	arma::vec diffne = arma::diff ( ne )  ;
	arma::vec grs = (diffne / ne.head( diffne.size() ) ) / dh ; //
	arma::vec diffgrs = arma::diff( grs ) ; 
	double xsd = sqrt( dh / tau ); 
	double ll = 0.; 
	for (int i = 0; i < diffgrs.size(); i++){
		ll += R::dnorm( diffgrs(i), diff_zxb(i), xsd, 1 ); 
	}
	return ll;
}

// TODO speed optimisations; do not recompute likelihood for all increments when only one component changes


////////////////////////////////////////////////////////////////////////
// true co likelihood based on intervals
double co_loglik2( arma::vec ne, arma::vec nco ,double dh, arma::mat lterms  ){
	arma::vec lls( ne.size(), fill::zeros);
	for (int i = 0; i< ne.size(); i++){
		lls(i) = lterms(i,0);
		lls(i) +=  log( 1./ne(i) ) * nco(i) ; 
		lls(i) -= lterms(i,1)  / ne(i); 
	}
	return sum( lls ) ; 
}

//~ mh_sample_ne1_2( ( ne) , rev( tredat$nco ),  tau, mapfit$sigma, dh, lterms) 
//[[Rcpp::export()]]
arma::vec mh_sample_ne1_2( arma::vec fwdnevec, arma::vec fwdnco , double tau , arma::vec prop_sigma, double dh, arma::mat lterms){
	arma::vec logne = log ( fwdnevec ) ;
	double ll = rbm_loglik1_2( fwdnevec, tau, dh ) + co_loglik2( fwdnevec, fwdnco, dh, lterms ) ;
	double propll;
	arma::vec proplogne = log( fwdnevec ); 
	for (int i = 0; i < logne.size() ; i++){
		//~ proplogne = logne; 
		proplogne(i) =  Rf_rnorm(  logne(i), prop_sigma(i) ) ; 
		propll = rbm_loglik1_2( exp(proplogne), tau, dh ) + co_loglik2( exp(proplogne), fwdnco, dh, lterms ) ;
		
		if ( Rf_runif(0.,1.) < exp( propll - ll)){
			logne(i) = proplogne(i); 
			ll = propll; 
		} else {
			proplogne(i) = logne(i) ;
		}
	}
	return exp(logne) ;
}

// uses covars: 
//[[Rcpp::export()]]
arma::vec mh_sample_ne2_2( arma::vec fwdnevec, arma::vec fwdnco , double tau , arma::vec prop_sigma, double dh, arma::vec zxb, arma::mat lterms ){
	// mean diference predicted by covariates: 	
	arma::vec dzxb = arma::diff( zxb ); 
	for (int i = 0; i < dzxb.size(); i++){
		if ( NumericVector::is_na(dzxb(i))){
			dzxb(i) = 0.; 
		}
	}
	
	
	arma::vec logne = log ( fwdnevec ) ;
	double ll = rbm_loglik2_2( fwdnevec, tau, dh, dzxb ) + co_loglik2( fwdnevec, fwdnco, dh, lterms ) ;
	double propll;
	arma::vec proplogne = log( fwdnevec ); 
	for (int i = 0 ; i < logne.size() ; i++){
		//~ proplogne = logne; 
		proplogne(i) =  Rf_rnorm(  logne(i), prop_sigma(i)  ) ; 
		propll = rbm_loglik2_2( exp( proplogne ), tau, dh, dzxb ) + co_loglik2( exp(proplogne), fwdnco, dh, lterms ) ;
		
		if ( Rf_runif(0.,1.) < exp( propll - ll)){
			logne(i) = proplogne(i); 
			ll = propll; 
		} else {
			proplogne(i) = logne(i) ;
		}
	}
	
	return exp(logne) ;
}

