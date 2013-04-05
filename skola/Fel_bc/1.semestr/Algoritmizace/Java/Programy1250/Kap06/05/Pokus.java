/* Soubor Kao06\04\Pokus.java
 * P��klad na pou�it� pole
 * Metoda pocetDnu() t��dy Kalendar vypo�te po�adov� ��slo dne v roce
 *
 * T��da Test slou�� k testov�n�
 */

class Kalendar {
 int mesice[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; // Po�ty dn� v m�s�c�
 int dny[];		// Po�ty dn� od po��tku roku na konci p�edchoz�ho m�s�ce
 Kalendar() {		// Konstruktor	
  dny = new int[12];	// Vytvo�� pole dny
  int i = 1;		
  dny[0] = 0;		// Vypo�te hodnoty v n�m: Po�et dn�u do po��tku ledna je 0, do po��tku �nora je 31 atd
  while(i < dny.length)
  {
    dny[i] = dny[i-1]+mesice[i-1];
    i++;
  }
 }

 boolean prestupny(int rok) 	// Zjist�, zda je rok p�estupn�
 {				// Tj. bu� d�liteln� 4 a ne 100  nebo d�liteln� 400
    if(((rok%4 == 0) && (rok % 100)!=0) || (rok % 400 == 0)) return true;
    else return false;
 }

 int cisloDne(int den, int mesic, int rok)	// Vypo�te po�adov� ��slo dne v roce
 {
    int d = dny[mesic-1]+den;			// Po�et dn� do po��tku m�s�ce + ��slo dne v m�s�ci (datum v m�s�ci)
    if(prestupny(rok) && mesic>2) d++;		// Je=li rok p�estupn�, nutno od 1.3. p�idat 1 za 29. �nor
    return d;
 }
}

public class Pokus {

  public Pokus() {
  }
  public static void main(String[] args) {
    Pokus pokus1 = new Pokus();
    Kalendar kal = new Kalendar();
    System.out.println("" + kal.cisloDne(1,3,2001));
  }
}