package konvertor;

/*
 * T��da pro p�enos informac� o v�jimce
 * p�i �patn�m z�kladu soustavy
 */

public class WrongBaseException extends RuntimeException {
  public WrongBaseException() {}
  public WrongBaseException(String s) {super(s);}
}