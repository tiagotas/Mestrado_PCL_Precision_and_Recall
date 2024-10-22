// CenariosPrecisionAndRecall.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"



// Declarando a nuvem do cenário.
pcl::PointCloud <pcl::PointXYZRGB>::Ptr cloud_org(new pcl::PointCloud <pcl::PointXYZRGB>);

// Declarando o padrao ouro.
pcl::PointCloud <pcl::PointXYZRGB>::Ptr cloud_pb(new pcl::PointCloud <pcl::PointXYZRGB>);


// Declarando o padrao ouro com regioes hipoidroticas.
pcl::PointCloud <pcl::PointXYZRGB>::Ptr cloud_pb_hipoidroticas(new pcl::PointCloud <pcl::PointXYZRGB>);


// Declarando o Padrão Ouro (somente anidrose) em Escala de Cinza.
pcl::PointCloud <pcl::PointXYZI>::Ptr cloud_ref;

// Declarando o Padrão Ouro com Hipoidrose em Escala de Cinza.
pcl::PointCloud <pcl::PointXYZI>::Ptr cloud_ref_hipoidrose;

// Declarando a nuvem de pontos do cenario corrente.
pcl::PointCloud <pcl::PointXYZI>::Ptr cloud_pred;


// Declarando a variável que contém os dados do precision and recall das duas comparações.
std::stringstream valores_calculados;



/**
* Converte as nuvens para tons de cinza, adicionando a intensidade que é usada como critério de
* cálculo entre do TP, TN, FP e FN.
*/
pcl::PointCloud<pcl::PointXYZI>::Ptr cloudRGB2GRAY(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud) {
	pcl::PointCloud<pcl::PointXYZI>::Ptr cloud_gray(new pcl::PointCloud<pcl::PointXYZI>);
	cloud_gray->height = cloud->height;
	cloud_gray->width = cloud->width;

	for (pcl::PointCloud<pcl::PointXYZRGB>::iterator it = cloud->begin(); it != cloud->end(); it++) {
		// Color conversion
		cv::Mat pixel(1, 1, CV_8UC3, cv::Scalar(it->r, it->g, it->b));
		cv::Mat temp;
		cv::cvtColor(pixel, temp, CV_RGB2GRAY);

		pcl::PointXYZI pointI;
		pointI.x = it->x;
		pointI.y = it->y;
		pointI.z = it->z;
		pointI.intensity = temp.at<uchar>(0, 0);

		cloud_gray->push_back(pointI);

	}
	return cloud_gray;
}



/**
 * Função que calcula e grava o arquivo txt com o nro do cenario e os dados do Precision and Recall.
 */
void save_txt(int cenario) {

	std::stringstream nome_arquivo;
	nome_arquivo << "Dados_Precision_and_Recall_Cenario_" << cenario << ".txt";

	 

	std::stringstream ss;
	ss << "            Dados Precision and Recall do Cenario " << cenario << std::endl << std::endl;
	

	ss << valores_calculados.str();



	std::cout << "  - Gravando os valores calculados no aquivo... ";

	FILE * pFile;
	pFile = fopen(nome_arquivo.str().c_str(), "w");
	if (pFile != NULL)
	{
		fputs(ss.str().c_str(), pFile);
		fclose(pFile);
	}

	std::cout << "OK " << std::endl << std::endl;
	std::cout << ss.str();
}



/**
 * Função que computa dos dados de TP, TN, FP, FN entre as duas nuvens.
 */
void cloud_compare(int scenario, pcl::PointCloud <pcl::PointXYZI>::Ptr cloud_ref, pcl::PointCloud <pcl::PointXYZI>::Ptr cloud_pred) {

	
	float tn = 0.f, tp = 0.f, fn = 0.f, fp = 0.f;
	int hidrotica_pontos = 0;

	pcl::KdTreeFLANN<pcl::PointXYZI> tree_ref;
	tree_ref.setInputCloud(cloud_ref);

	std::vector<int> nn_indices(1);
	std::vector<float> nn_dists(1);



	// Comparando as duas nuvens de pontos.
	for (pcl::PointCloud<pcl::PointXYZI>::iterator it = cloud_pred->begin(); it != cloud_pred->end(); it++) {
		tree_ref.nearestKSearch(*it, 1, nn_indices, nn_dists);

		float i_ref = cloud_ref->points[nn_indices[0]].intensity;
		float i_pred = it->intensity;

		if (i_ref == 255 & i_pred == 255) tp++;
		else if (i_ref == 0 & i_pred == 0) tn++;
		else if (i_ref == 0 & i_pred == 255) fp++;
		else if (i_ref == 255 & i_pred == 0) fn++;


		if (i_ref == 255 & i_pred == 255) hidrotica_pontos++;
	}



	// Calculando Precision and Recall.
	float precision = tp / (tp + fp);
	float recall = tp / (tp + fn);
	float accu = (tp + tn) / (tp + tn + fp + fn);
	float tpr = tp / (tp + fn);
	float tnr = tn / (tn + fp);
	float f_m = 2 * ((precision*recall) / (precision + recall));


	// Calculando % das áreas de anidrose e hipoidrose juntas.
	int total_pontos = cloud_org->size();

	float porcentagem_hidrotica = (hidrotica_pontos * 100) / total_pontos;
	float porcentagem_anidrose_hipoidrotica = 100 - porcentagem_hidrotica;


	valores_calculados << "   - Precision: " << precision << std::endl;
	valores_calculados << "   - Recall: " << recall << std::endl;
	valores_calculados << "   - True Positive Rate: " << tpr << std::endl;
	valores_calculados << "   - True Negative Rate: " << tnr << std::endl;
	valores_calculados << "   - Accuracy: " << accu << std::endl;
	valores_calculados << "   - F measure: " << f_m << std::endl << std::endl;

	valores_calculados << "   - Areas Hidroticas: " << porcentagem_hidrotica << "%" << std::endl;
	valores_calculados << "   - Areas Anidrose e Hipoidroticas: " << porcentagem_anidrose_hipoidrotica << "%" << std::endl;
}



/**
 * Função que cria cada cenário, chamará a função cloud_compare. 
 */
void make_scenario(int scenario, std::string ply_scenario) {

	// Zerando os valores que serão gravados no arquivo txt...
	valores_calculados.str(std::string());
	
	std::cout << std::endl << std::endl << "  - Tentando ler a nuvem de pontos do cenario  " << scenario << "... ";

	pcl::PLYReader reader;

	if (reader.read(ply_scenario, *cloud_org) == -1) {
		std::cout << "Nao foi possivel ler a nuvem do cenario." << std::endl;
	}

	std::cout << "OK " << std::endl;





	std::cout << "  - Calculando a intensidade da nuvem de pontos do cenario... ";
	cloud_pred = cloudRGB2GRAY(cloud_org);
	std::cout << "OK " << std::endl << std::endl;
	


	std::cout << "  - Calculando Precision and Recall para as areas de anidrose... ";
	valores_calculados << "    Precision and Recall para as areas de anidrose: " << std::endl << std::endl;
	cloud_compare(scenario, cloud_ref, cloud_pred);
	std::cout << "OK " << std::endl;

	
	//valores_calculados << "___________________________________________________________" << std::endl << std::endl;

	valores_calculados << std::endl << std::endl;



	std::cout << "  - Calculando Precision and Recall para as areas de anidrose e hipoidroticas... ";
	valores_calculados << "    Precision and Recall para as areas de anidrose e hipoidroticas: " << std::endl << std::endl;
	cloud_compare(scenario, cloud_ref_hipoidrose, cloud_pred);
	std::cout << "OK " << std::endl << std::endl;

	save_txt(scenario);

	std::cout << std::endl << "___________________________________________________________" << std::endl;
}



/**
* Le a nuvem de pontos que simboliza o padrao ouro: Entra em PLY.
*/
void read_cloud_padrao_ouro()
{
	std::cout << "___________________________________________________________" << std::endl;
	std::cout << "                LENDO AS NUVENS DO PADRAO OURO             " << std::endl;

	std::cout << "  - Tentando ler a nuvem do padrao ouro somente das areas de anidrose... ";

	std::string padrao_ouro_anidrose = "PLY_PadraoOuro/padrao_ouro_anidrose.ply";

	pcl::PLYReader reader;

	// Lendo a nuvem do Padrão Ouro considerando apenas as áreas de Anidrose.
	if (reader.read(padrao_ouro_anidrose, *cloud_pb) == -1) { //lucy_pb.ply
		std::cout << std::endl << "  - Erro ao ler a nuvem de pontos do padrao ouro das areas de anidrose." << std::endl;
	}

	std::cout << "OK " << std::endl;


	std::cout << "  - Calculando a intensidade da nuvem de pontos das areas de anidrose... ";
	cloud_ref = cloudRGB2GRAY(cloud_pb);
	std::cout << "OK " << std::endl << std::endl;


	
	std::cout << "  - Tentando ler a nuvem do padrao ouro areas de anidrose com hipoidroticas... ";

	std::string padrao_ouro_com_hipoidrotica = "PLY_PadraoOuro/padrao_ouro_anidrose_com_hipoidrotica.ply";

	// Lendo a nuvem do Padrão Ouro considerando apenas as áreas de Anidrose com Hipoidrose.
	if (reader.read(padrao_ouro_com_hipoidrotica, *cloud_pb_hipoidroticas) == -1) { //lucy_pb.ply
		std::cout << std::endl << "Erro ao ler a nuvem de pontos do padrao ouro das areas de anidrose com hipoidroticas." << std::endl;
	}

	std::cout << "OK " << std::endl;


	std::cout << "  - Calculando a intensidade da nuvem de pontos das areas de anidrose com hipoidroticas... ";
	cloud_ref_hipoidrose = cloudRGB2GRAY(cloud_pb_hipoidroticas);
	std::cout << "OK " << std::endl << std::endl;
}






int main()
{
	std::cout << "PROGRAMA DE COMPARACAO DE PRECISION AND RECALL INICIADO! " << std::endl << std::endl;


	// Lendo a núvem do padrao ouro.
	read_cloud_padrao_ouro();



	std::cout << "___________________________________________________________" << std::endl;
	std::cout << "           INICIANDO O CALCULO PARA OS CENARIOS            ";



	/**
	* Avaliando a segmentação do Region Growing em LAB com a variação do MinClusterSize.
	*/
	/*make_scenario(10, "PLY_Comparacao/LAB/minClusterSize/paciente_segmentado_cenario_10_pb.ply");
	make_scenario(11, "PLY_Comparacao/LAB/minClusterSize/paciente_segmentado_cenario_11_pb.ply");
	make_scenario(12, "PLY_Comparacao/LAB/minClusterSize/paciente_segmentado_cenario_12_pb.ply");
	make_scenario(13, "PLY_Comparacao/LAB/minClusterSize/paciente_segmentado_cenario_13_pb.ply");
	make_scenario(14, "PLY_Comparacao/LAB/minClusterSize/paciente_segmentado_cenario_14_pb.ply");
	make_scenario(15, "PLY_Comparacao/LAB/minClusterSize/paciente_segmentado_cenario_15_pb.ply");
	make_scenario(16, "PLY_Comparacao/LAB/minClusterSize/paciente_segmentado_cenario_16_pb.ply");
	make_scenario(17, "PLY_Comparacao/LAB/minClusterSize/paciente_segmentado_cenario_17_pb.ply");
	make_scenario(18, "PLY_Comparacao/LAB/minClusterSize/paciente_segmentado_cenario_18_pb.ply");
	make_scenario(19, "PLY_Comparacao/LAB/minClusterSize/paciente_segmentado_cenario_19_pb.ply");
	make_scenario(20, "PLY_Comparacao/LAB/minClusterSize/paciente_segmentado_cenario_20_pb.ply");
	make_scenario(21, "PLY_Comparacao/LAB/minClusterSize/paciente_segmentado_cenario_21_pb.ply");*/


	/**
	* Avaliando a segmentação do Region Growing em LAB com a variação do PointColorThreshold.
	*/
	/*make_scenario(22, "PLY_Comparacao/LAB/PointColorThreshold/paciente_segmentado_cenario_22_pb.ply");
	make_scenario(23, "PLY_Comparacao/LAB/PointColorThreshold/paciente_segmentado_cenario_23_pb.ply");
	make_scenario(24, "PLY_Comparacao/LAB/PointColorThreshold/paciente_segmentado_cenario_24_pb.ply");
	make_scenario(25, "PLY_Comparacao/LAB/PointColorThreshold/paciente_segmentado_cenario_25_pb.ply");
	make_scenario(26, "PLY_Comparacao/LAB/PointColorThreshold/paciente_segmentado_cenario_26_pb.ply");
	make_scenario(27, "PLY_Comparacao/LAB/PointColorThreshold/paciente_segmentado_cenario_27_pb.ply");
	make_scenario(28, "PLY_Comparacao/LAB/PointColorThreshold/paciente_segmentado_cenario_28_pb.ply");
	make_scenario(29, "PLY_Comparacao/LAB/PointColorThreshold/paciente_segmentado_cenario_29_pb.ply");
	make_scenario(30, "PLY_Comparacao/LAB/PointColorThreshold/paciente_segmentado_cenario_30_pb.ply");
	make_scenario(31, "PLY_Comparacao/LAB/PointColorThreshold/paciente_segmentado_cenario_31_pb.ply");
	make_scenario(32, "PLY_Comparacao/LAB/PointColorThreshold/paciente_segmentado_cenario_32_pb.ply");*/


	/**
	* Avaliando a segmentação do Region Growing em LAB com a variação do RegionColorThreshold.
	*/
	/*make_scenario(33, "PLY_Comparacao/LAB/RegionColorThreshold/paciente_segmentado_cenario_33_pb.ply");
	make_scenario(34, "PLY_Comparacao/LAB/RegionColorThreshold/paciente_segmentado_cenario_34_pb.ply");
	make_scenario(35, "PLY_Comparacao/LAB/RegionColorThreshold/paciente_segmentado_cenario_35_pb.ply");
	make_scenario(36, "PLY_Comparacao/LAB/RegionColorThreshold/paciente_segmentado_cenario_36_pb.ply");
	make_scenario(37, "PLY_Comparacao/LAB/RegionColorThreshold/paciente_segmentado_cenario_37_pb.ply");
	make_scenario(38, "PLY_Comparacao/LAB/RegionColorThreshold/paciente_segmentado_cenario_38_pb.ply");
	make_scenario(39, "PLY_Comparacao/LAB/RegionColorThreshold/paciente_segmentado_cenario_39_pb.ply");
	make_scenario(40, "PLY_Comparacao/LAB/RegionColorThreshold/paciente_segmentado_cenario_40_pb.ply");
	make_scenario(41, "PLY_Comparacao/LAB/RegionColorThreshold/paciente_segmentado_cenario_41_pb.ply");
	make_scenario(42, "PLY_Comparacao/LAB/RegionColorThreshold/paciente_segmentado_cenario_42_pb.ply");
	make_scenario(43, "PLY_Comparacao/LAB/RegionColorThreshold/paciente_segmentado_cenario_43_pb.ply");*/



	/**
	* Avaliando a mistura dos melhores cenários dos três grupos anteriores.
	*/
	//make_scenario(44, "PLY_Comparacao/LAB/MelhoresCenarios/paciente_segmentado_cenario_44_pb.ply");
	make_scenario(45, "PLY_Comparacao/LAB/MelhoresCenarios/paciente_segmentado_cenario_45_pb.ply");
	//make_scenario(46, "PLY_Comparacao/LAB/MelhoresCenarios/paciente_segmentado_cenario_46_pb.ply");











































	/**
	 * Avaliando a segmentação do Region Growing com a variação do MinClusterSize.
	 */
	/*make_scenario(10, "PLY_Comparacao/MinClusterSize/paciente_segmentado_cenario_10_pb.ply");
	make_scenario(11, "PLY_Comparacao/MinClusterSize/paciente_segmentado_cenario_11_pb.ply");
	make_scenario(12, "PLY_Comparacao/MinClusterSize/paciente_segmentado_cenario_12_pb.ply");
	make_scenario(13, "PLY_Comparacao/MinClusterSize/paciente_segmentado_cenario_13_pb.ply");
	make_scenario(14, "PLY_Comparacao/MinClusterSize/paciente_segmentado_cenario_14_pb.ply");
	make_scenario(15, "PLY_Comparacao/MinClusterSize/paciente_segmentado_cenario_15_pb.ply");
	make_scenario(16, "PLY_Comparacao/MinClusterSize/paciente_segmentado_cenario_16_pb.ply");
	make_scenario(17, "PLY_Comparacao/MinClusterSize/paciente_segmentado_cenario_17_pb.ply");
	make_scenario(18, "PLY_Comparacao/MinClusterSize/paciente_segmentado_cenario_18_pb.ply");
	make_scenario(19, "PLY_Comparacao/MinClusterSize/paciente_segmentado_cenario_19_pb.ply");
	make_scenario(20, "PLY_Comparacao/MinClusterSize/paciente_segmentado_cenario_20_pb.ply");
	make_scenario(21, "PLY_Comparacao/MinClusterSize/paciente_segmentado_cenario_21_pb.ply");*/




	/**
	 * Avaliando a segmentação do Region Growing com a variação do PointColorThreshold.
	 */
	 /*make_scenario(22, "PLY_Comparacao/PointColorThreshold/paciente_segmentado_cenario_22_pb.ply");
	make_scenario(23, "PLY_Comparacao/PointColorThreshold/paciente_segmentado_cenario_23_pb.ply");
	make_scenario(24, "PLY_Comparacao/PointColorThreshold/paciente_segmentado_cenario_24_pb.ply");
	make_scenario(25, "PLY_Comparacao/PointColorThreshold/paciente_segmentado_cenario_25_pb.ply");
	make_scenario(26, "PLY_Comparacao/PointColorThreshold/paciente_segmentado_cenario_26_pb.ply");
	make_scenario(27, "PLY_Comparacao/PointColorThreshold/paciente_segmentado_cenario_27_pb.ply");
	make_scenario(28, "PLY_Comparacao/PointColorThreshold/paciente_segmentado_cenario_28_pb.ply");
	make_scenario(29, "PLY_Comparacao/PointColorThreshold/paciente_segmentado_cenario_29_pb.ply");
	make_scenario(30, "PLY_Comparacao/PointColorThreshold/paciente_segmentado_cenario_30_pb.ply");
	make_scenario(31, "PLY_Comparacao/PointColorThreshold/paciente_segmentado_cenario_31_pb.ply");
	make_scenario(32, "PLY_Comparacao/PointColorThreshold/paciente_segmentado_cenario_32_pb.ply");*/


	 /**
	  * Avaliando a segmentação do Region Growing com a variação do RegionColorThreshold.
	  */
	 /*make_scenario(33, "PLY_Comparacao/RegionColorThreshold/paciente_segmentado_cenario_33_pb.ply");
	 make_scenario(34, "PLY_Comparacao/RegionColorThreshold/paciente_segmentado_cenario_34_pb.ply");
	 make_scenario(35, "PLY_Comparacao/RegionColorThreshold/paciente_segmentado_cenario_35_pb.ply");
	 make_scenario(36, "PLY_Comparacao/RegionColorThreshold/paciente_segmentado_cenario_36_pb.ply");
	 make_scenario(37, "PLY_Comparacao/RegionColorThreshold/paciente_segmentado_cenario_37_pb.ply");
	 make_scenario(38, "PLY_Comparacao/RegionColorThreshold/paciente_segmentado_cenario_38_pb.ply");
	 make_scenario(39, "PLY_Comparacao/RegionColorThreshold/paciente_segmentado_cenario_39_pb.ply");
	 make_scenario(40, "PLY_Comparacao/RegionColorThreshold/paciente_segmentado_cenario_40_pb.ply");
	 make_scenario(41, "PLY_Comparacao/RegionColorThreshold/paciente_segmentado_cenario_41_pb.ply");
	 make_scenario(42, "PLY_Comparacao/RegionColorThreshold/paciente_segmentado_cenario_42_pb.ply");
	 make_scenario(43, "PLY_Comparacao/RegionColorThreshold/paciente_segmentado_cenario_43_pb.ply");*/



	 /**
	  * Avaliando a mistura dos melhores cenários dos três grupos anteriores.
	  */
	/*make_scenario(44, "PLY_Comparacao/MelhoresCenarios/paciente_segmentado_cenario_44_pb.ply");
	make_scenario(45, "PLY_Comparacao/MelhoresCenarios/paciente_segmentado_cenario_45_pb.ply");
	make_scenario(46, "PLY_Comparacao/MelhoresCenarios/paciente_segmentado_cenario_46_pb.ply");*/



	/**
	 * Avaliando a segmentaçã feita com k-means.
	 */
	//make_scenario(1, "PLY_Comparacao/kmeans_lucas/k-means-para-precison-and-recall-orientacao-correta.ply");


















	//

	//make_scenario(2, "PLY_PadraoOuro/padrao_ouro_anidrose_com_hipoidrotica.ply");
	//make_scenario(2, "PLY_PadraoOuro/padrao_ouro_anidrose.ply");



	//Cenários
	/*make_scenario(1, "lucy_fake_cenario_1.ply");
	make_scenario(2, "lucy_fake_cenario_1.ply");
	make_scenario(3, "lucy_fake_cenario_1.ply");
	make_scenario(4, "lucy_fake_cenario_1.ply");
	make_scenario(5, "lucy_fake_cenario_1.ply");
	/*make_scenario(6, "paciente_cenario_6.ply");
	make_scenario(7, "paciente_cenario_7.ply");
	make_scenario(8, "paciente_cenario_8.ply");
	make_scenario(9, "paciente_cenario_9.ply");
	make_scenario(10, "paciente_cenario_10.ply");*/

	//make_scenario(15, "paciente_pb_preliminar_cenario_2.ply"); 

	//make_scenario(21, "experimentos_preliminares/paciente_pb_cenario_1.ply");
	//make_scenario(22, "experimentos_preliminares/paciente_pb_cenario_2.ply");
	//make_scenario(23, "experimentos_preliminares/paciente_pb_cenario_3.ply");
	//make_scenario(24, "experimentos_preliminares/paciente_pb_cenario_4.ply");
	//make_scenario(25, "experimentos_preliminares/paciente_pb_cenario_5.ply");

	//k-means
	//make_scenario(11, "experimento-kmeans-lucas/kmeans-lucas.ply");


	// watershed
	//make_scenario(12, "paciente_watershed.ply");


	std::cout << "                 Fim da Execucao do Programa. " << std::endl << std::endl;
	// Aguarda a finalização do programa.*/
	getchar();

    return 0;
}