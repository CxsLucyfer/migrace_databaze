/* Soubor Kap13\05\Test.java
 * P��klad vstupu z textov�ho souboru
 * lze pou��t ve v�ech verz�ch JDK
 */

import java.io.*;

public class Test {

  public Test() {  }
  public static int maxVSouboru(String jmeno)  throws Exception
  {
    File f = new File(jmeno);
    if(!f.exists()) throw new IOException("soubor neexistuje");
    int max = Integer.MIN_VALUE;           // Do�asn� maximum

    FileReader fr = new FileReader(f);
    BufferedReader bw = new BufferedReader(fr);

    String cisla;
    while ((cisla = bw.readLine()) != null)
    {
      // anal�za ��dku
      java.util.StringTokenizer st = new java.util.StringTokenizer(cisla);
      while(st.hasMoreTokens())
      {
        String s = st.nextToken();
        int i = Integer.parseInt(s);
        if(i > max) max = i;
      }
    }
    bw.close();
    return max;
  }
  public static void main(String[] args) throws Exception
  {
    System.out.print("Nejv�t�� ��slo v souboru: " + maxVSouboru("data1.dta"));
  }
}