#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>
#include "utils.h"

// Returns a CHARSXP
SEXP as_name(SEXP x) {
  switch(TYPEOF(x)) {
  case STRSXP:
    if (Rf_length(x) != 1)
      Rf_errorcall(R_NilValue, "LHS must evaluate to a single string");
    return STRING_ELT(x, 0);
  case SYMSXP:
    return PRINTNAME(x);
  case LANGSXP:
    if (!is_formula(x) || Rf_length(x) != 2)
      Rf_errorcall(R_NilValue, "RHS of LHS must be a single-sided formula");

    return as_name(rhs(x));
  default:
    Rf_errorcall(R_NilValue, "LHS must evaluate to a string or name");
  }
}

SEXP shallow_duplicate(SEXP list)
{
  if (TYPEOF(list) != LISTSXP)
    Rf_errorcall(R_NilValue, "'shallow_duplicate' can only handle LISTSXP objects for now.");
  SEXP new = R_NilValue;
  SEXP next, a;
  /**** Could be done in a single pass */
  /**** Don't need to protect since CONS does */
  for (next = list; next != R_NilValue; next = CDR(next))
    new = Rf_cons(R_NilValue, new);
  for (next = list, a = new;
       next != R_NilValue;
       next = CDR(next), a = CDR(a)) {
    SETCAR(a, CAR(next));
    SET_TAG(a, TAG(next));
    /**** ATTRIB?? */
    SET_NAMED(CAR(a), 2); /* may not be necessary */
  }
  return new;
}

SEXP lhs_name(SEXP x) {
  if (TYPEOF(x) != VECSXP)
    Rf_errorcall(R_NilValue, "`x` must be a list (not a %s)", Rf_type2char(TYPEOF(x)));

  int n = Rf_length(x);
  SEXP x2 = PROTECT(shallow_duplicate(x));

  SEXP names = Rf_getAttrib(x2, R_NamesSymbol);
  if (names == R_NilValue) {
    names = Rf_allocVector(STRSXP, n);
    Rf_setAttrib(x2, R_NamesSymbol, names);
  }

  for (int i = 0; i < n; ++i) {
    SEXP xi = VECTOR_ELT(x2, i);
    if (!is_formula(xi) || Rf_length(xi) != 3)
      continue;

    // set name
    SEXP name = PROTECT(Rf_eval(lhs(xi), env(xi)));
    if (TYPEOF(name) != NILSXP)
      SET_STRING_ELT(names, i, as_name(name));
    UNPROTECT(1);

    // replace with RHS of formula
    SET_VECTOR_ELT(x2, i, make_formula1(CADDR(xi), env(xi)));
  }

  UNPROTECT(1);
  return x2;
}
