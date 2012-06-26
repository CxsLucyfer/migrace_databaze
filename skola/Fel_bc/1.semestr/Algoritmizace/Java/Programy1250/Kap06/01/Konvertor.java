/* Soubor Kap06\01\Konvertor.java
 * T��da pro konverzi cel�ho ��sla na znakov� �et�zec 
 * umo��uje vytvo�it ��selnou reprezentaci v ��seln� soustav� se z�kladem 2 - 36
 * ��slice jsou vyj�d�eny znaky 0, 1, ... ,9, A, B, ... , Z
 *
 * Z�klad soustavy je parametrem konstruktoru, 
 * konvertovan� ��slo typu long je parametrem metody konverze.
 * Pou�it� - konverze ��sla x do �estn�ctkov� soustavy:
 *
 * Konvertor k = new Konvertor(16);
 * string s = k.konverze(x)
 */

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
	zaklad = baze;  
  }
  public static void main(String[] args) {
    //Konvertor k = new Konvertor(16);
    //System.out.println(k.konverze(-0xFFFFFFFFL));
    System.out.println((new Konvertor(2)).konverze(15));
  }
}