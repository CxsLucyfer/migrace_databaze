/* Soubor Kap13\06\Hlod.java
 * P��klad za z�pis �esk�ho textu do souboru v k�dov�n� Latin 2
 * Citovan� text poch�z� z vys�l�n� �esk� televize o v�noc�ch r. 2000
 */

import java.io.*;

public class Hlod{

  public static void main(String[] args) throws Exception{
    File f = new File("data.dta");
    if(!f.exists()) f.createNewFile();

    FileOutputStream fos = new FileOutputStream(f);
    OutputStreamWriter osw = new OutputStreamWriter(fos, "Cp852");
    PrintWriter pw = new PrintWriter(osw);
    pw.println("Gener�ln� �editel �T Ji�� Hoda� se obr�til\n"+
		"na Radu �esk� republiky pro rozhlasov� \na televizn� " +
		"vys�l�n� se ��dost� o rozhodnut�, \nkter� program �T " +
		"je leg�ln�m a autorizovan�m \nprogramem v souladu se z�konem " +
		"o �esk� \ntelevizi a kter� nikoli. Do rozhodnut� Rady \nvys�l� �T " +
		"jako sv�j program toto sd�len�.");

    pw.close();
  }
}