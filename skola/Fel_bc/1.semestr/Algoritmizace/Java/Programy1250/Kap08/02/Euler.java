/* Soubor Kap08\02\Euler .java
 * V�po�et Eulerova ��sla s p�esnopst� eps
 * 
 * p��klad na cyklus do-while
 * 
 * vytiskne vypo�tenou hodnotu a 
 * spr�vnou hodnotu ulo�enou v syst�mov� konstant� Math.E
 */

public class Euler {

  public Euler() {
  }
  double pocitej(double eps)
  {
	double s = 2, d = 1;      // s je sou�et, d je s��tanec
	int i = 2;          	  // Po��t�, kolik�t� s��tanec
	do {
		d /= i++;         // Vypo�ti dal�� s��tanec
		s += d;           // a p�i�ti ho k sou�tu
	} while(d > eps);         // Je-li men�� ne� eps, hotovo
	return s;
  }
  public static void main(String[] args) {
    Euler e = new Euler();
    System.out.println(e.pocitej(1e-5));
    System.out.println(Math.E);
  }
}