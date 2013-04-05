/* Soubor Kap12\04\Test852.java
 *  P��klad v�stupu do textov�ho souboru, �e�tina v k�dov� str�nkce 852
 * Zdrojov� text ale obsahuje �e�tinu v k�dov� str�nce 1250, tak�e 
 * pokud to budete p�ekl�dsat v jin�m prost�ed�, nutno pou��t p��kaz
 * javac -encoding Cp1250 Test852
 * V�stup bude v ka�d�m p��pad� obsahovat �e�tinu v k�dov� str�nce 852 (Latin 2)
 */

import java.io.*;

public class Test852 {

  public Test852() {  }

  public static void main(String[] args) throws Exception{
    Test852 t = new Test852();
    File f = new File("data.dta");
    if(!f.exists()) f.createNewFile();

    FileOutputStream fos = new FileOutputStream(f);
    OutputStreamWriter osw = new OutputStreamWriter(fos, "Cp852");
    PrintWriter pw = new PrintWriter(osw);
    for(int i = 0; i < 10; i++) pw.print(i+ " ");
    pw.println();
    pw.println(Math.PI);
    pw.println(Math.E);
    pw.println("�lu�ou�k� k�� p��ern� �p�l ��belsk� �dy");

    pw.close();
  }
}