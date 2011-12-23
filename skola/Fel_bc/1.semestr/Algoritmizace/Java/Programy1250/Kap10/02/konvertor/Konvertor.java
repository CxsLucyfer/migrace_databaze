/* Soubor Kap10\02\konvertor\Konvertor.java
 * T��da pro konverzi cel�ho ��sla na znakov� �et�zec
 * umo��uje vytvo�it ��selnou reprezentaci v ��seln� soustav� se z�kladem 2 - 36
 * ��slice jsou vyj�d�eny znaky 0, 1, ... ,9, A, B, ... , Z
 *
 * UPRAVEN� VERZE, KTER� P�I �PATN� ZADAN�M Z�KLADU SOUSTAVY VYVOL� V�JIMKU
 * 
 * Z�klad soustavy je parametrem konstruktoru,
 * konvertovan� ��slo typu long je parametrem metody konverze.
 * Pou�it� - konverze ��sla x do �estn�ctkov� soustavy:
 *
 * Konvertor k = new Konvertor(16);
 * string s = k.konverze(x)
 */

package konvertor;
public class Konvertor {
  private long zaklad;  // Z�klad ��seln� soustavy
  static String cislice = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"; // �et�zec obsahuj�c� v�echny ��slice, kter� se mohou vyskytnout v soustav�ch o z�kladu od 2 do 36
  public String konverze(long n){
    StringBuffer s = new StringBuffer(""); // Zde budeme vytv��et znakovou reprezentaci ��sla
    boolean zaporne = false;
    if(n==0)
    {                   // Je-li to nula, vra� �et�zec "0" (je stejn� ve v�ech soustav�ch)
      return "0";
    }
    if (n < 0)
    {
      zaporne = true;
      n = -n;
    };
    while(n != 0){
      s.append(cislice.charAt((int)(n%zaklad)));
      n /= zaklad;
    }
    if(zaporne)s.append('-');
    return new String(s.reverse());
  }

  public Konvertor(int baze) { // Parametrem je z�klad soustavy
	if(baze < 2 || baze > 36) throw new WrongBaseException(""+baze);
        else zaklad = baze;
  }
  public static void main(String[] args) {
    System.out.println((new Konvertor(2)).konverze(15));
  }
}