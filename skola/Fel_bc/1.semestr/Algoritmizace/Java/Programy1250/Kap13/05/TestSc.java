// Soubor Kap13\05\TestSc.java
// Ukazuje pou�it� t��dy java.util.Scanner
// pro �ten� textov�ho souboru
// 
// Pouze pro JDK 5
//
import java.util.Scanner;
import java.io.*;

public class TestSc {
  public static int maxVSouboru(String jmeno)  throws Exception
  {
    File f = new File(jmeno);
    if(!f.exists()) throw new IOException("soubor neexistuje");
    int max = Integer.MIN_VALUE;           // Do�asn� maximum

    Scanner skan = new Scanner(f);         // P�ipoj�me skener k souboru
    while (skan.hasNextInt())              // �ti, dokud je co
    {
      int n = skan.nextInt();
      if(n > max) max = n;
    }
    return max;
  }
  public static void main(String[] args) throws Exception
  {
    System.out.print("Nejv�t�� ��slo v souboru: " + maxVSouboru("data1.dta"));
  }
}
