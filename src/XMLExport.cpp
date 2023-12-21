#include "XMLExport.h"
#include "SharedFunctions.h"

#include <format>
#include <string>
#include <filesystem>

int exportToXML(std::string outputFolder, std::string objectName, std::vector<PolygonStruct>& polygons, std::vector<Material>& materials)
{
    // This stuff is mostly just interfacing with tinyxml2, not really too much to say here

	int returnValue = 0;

	struct tm* timePointer;
	std::time_t currentTime = std::time(0);
	char timeString[100];
	timePointer = localtime(&currentTime);
	strftime(timeString, 100, "%FT%T", timePointer);

	tinyxml2::XMLDocument outputDAE;

	tinyxml2::XMLDeclaration* header = outputDAE.NewDeclaration("xml version = \"1.0\" encoding = \"UTF-8\" standalone = \"no\"");
	outputDAE.LinkEndChild(header);

	tinyxml2::XMLElement* rootNode = outputDAE.NewElement("COLLADA");
	rootNode->SetAttribute("xmlns", "http://www.collada.org/2005/11/COLLADASchema");
	rootNode->SetAttribute("version", "1.4.1");
	tinyxml2::XMLElement* asset = outputDAE.NewElement("asset");
	tinyxml2::XMLElement* contributor = outputDAE.NewElement("contributor");
	tinyxml2::XMLElement* author = outputDAE.NewElement("author");
	author->SetText("Crystal Dynamics");
	tinyxml2::XMLElement* authoring_tool = outputDAE.NewElement("authoring_tool");
	authoring_tool->SetText("Gex 2 PS1 Model Exporter");
	contributor->LinkEndChild(author);
	contributor->LinkEndChild(authoring_tool);
	tinyxml2::XMLElement* creationDate = outputDAE.NewElement("created");
	creationDate->SetText(timeString);
	tinyxml2::XMLElement* modifiedDate = outputDAE.NewElement("modified");
	modifiedDate->SetText(timeString);
	tinyxml2::XMLElement* unit = outputDAE.NewElement("unit");
	unit->SetAttribute("name", "meter");
	unit->SetAttribute("meter", "1");
	tinyxml2::XMLElement* up_axis = outputDAE.NewElement("up_axis");
	up_axis->SetText("Z_UP");
	asset->LinkEndChild(contributor);
	asset->LinkEndChild(creationDate);
	asset->LinkEndChild(modifiedDate);
	asset->LinkEndChild(unit);
	asset->LinkEndChild(up_axis);
	rootNode->LinkEndChild(asset);

	tinyxml2::XMLElement* library_images = outputDAE.NewElement("library_images");
	tinyxml2::XMLElement* library_effects = outputDAE.NewElement("library_effects");
	tinyxml2::XMLElement* library_materials = outputDAE.NewElement("library_materials");
	tinyxml2::XMLElement* library_geometries = outputDAE.NewElement("library_geometries");

	tinyxml2::XMLElement* library_visual_scenes = outputDAE.NewElement("library_visual_scenes");
	tinyxml2::XMLElement* visual_scene = outputDAE.NewElement("visual_scene");
	visual_scene->SetAttribute("id", objectName.c_str());
	visual_scene->SetAttribute("name", objectName.c_str());
	tinyxml2::XMLElement* nodeArmature = outputDAE.NewElement("node");
	nodeArmature->SetAttribute("id", std::format("{}node", objectName).c_str());
	nodeArmature->SetAttribute("name", objectName.c_str());
	nodeArmature->SetAttribute("type", "NODE");
	tinyxml2::XMLElement* matrixArmature = outputDAE.NewElement("matrix");
	matrixArmature->SetAttribute("sid", "matrix");
	matrixArmature->SetText("1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1");
	tinyxml2::XMLElement* nodeModel = outputDAE.NewElement("node");
	nodeModel->SetAttribute("id", std::format("{}node0", objectName).c_str());
	nodeModel->SetAttribute("name", objectName.c_str());
	nodeModel->SetAttribute("type", "NODE");
	tinyxml2::XMLElement* matrixModel = outputDAE.NewElement("matrix");
	matrixModel->SetAttribute("sid", "matrix");
	matrixModel->SetText("1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1");
	nodeModel->LinkEndChild(matrixModel);

    for (int m = 0; m < materials.size(); m++)
	{
		if (!materials[m].properlyExported)
			returnValue = 1;

        std::vector<PolygonStruct> meshPolygons;
		std::vector<Vertex> meshVertices;
        tinyxml2::XMLElement* xmlMaterial = outputDAE.NewElement("material");
        tinyxml2::XMLElement* xmlGeometry = outputDAE.NewElement("geometry");
        tinyxml2::XMLElement* xmlMesh = outputDAE.NewElement("mesh");

        exportTexture(outputDAE, library_images, materials[m], objectName);

        exportEffect(outputDAE, library_effects, materials[m], m, objectName);

        exportMaterial(outputDAE, xmlMaterial, library_materials, materials[m], m, objectName);

        // Geometry export includes positions, textures, colours, vertices, and polygons
        exportGeometry(outputDAE, polygons, xmlGeometry, xmlMesh, library_geometries, meshPolygons, meshVertices, materials[m], m, objectName);

        exportVisualScene(outputDAE, nodeModel, m, objectName);
    }

    rootNode->LinkEndChild(library_images);
	rootNode->LinkEndChild(library_effects);
	rootNode->LinkEndChild(library_materials);
	rootNode->LinkEndChild(library_geometries);

	nodeArmature->LinkEndChild(matrixArmature);
	nodeArmature->LinkEndChild(nodeModel);
	visual_scene->LinkEndChild(nodeArmature);
	library_visual_scenes->LinkEndChild(visual_scene);
	rootNode->LinkEndChild(library_visual_scenes);


	tinyxml2::XMLElement* scene = outputDAE.NewElement("scene");
	tinyxml2::XMLElement* instance_visual_scene = outputDAE.NewElement("instance_visual_scene");
	instance_visual_scene->SetAttribute("url", std::format("#{}", objectName).c_str());
	scene->LinkEndChild(instance_visual_scene);
	rootNode->LinkEndChild(scene);

	outputDAE.LinkEndChild(rootNode);

	if (std::filesystem::exists(outputFolder))
	{
        if (outputDAE.SaveFile(std::format("{}{}{}.dae", outputFolder, directorySeparator(), objectName).c_str()) == 0)
		    return returnValue;
	}

	// Could not export
	return 2;
}



int exportTexture(tinyxml2::XMLDocument& outputDAE, tinyxml2::XMLElement* library_images, Material exportMaterial, std::string objectName)
{
    if (exportMaterial.realMaterial && exportMaterial.properlyExported)
    {
        tinyxml2::XMLElement* image = outputDAE.NewElement("image");
        image->SetAttribute("id", std::format("{}-{}-diffuse-image", objectName, exportMaterial.textureID).c_str());
        tinyxml2::XMLElement* init_fromDiffuseImage = outputDAE.NewElement("init_from");
        init_fromDiffuseImage->SetText(std::format("{}-tex{}.png", objectName, exportMaterial.textureID + 1).c_str());
        image->LinkEndChild(init_fromDiffuseImage);
        library_images->LinkEndChild(image);
    }

    return 0;
}

int exportEffect(tinyxml2::XMLDocument& outputDAE, tinyxml2::XMLElement* library_effects, Material exportMaterial, int materialID, std::string objectName)
{
    if (exportMaterial.properlyExported)
    {
        tinyxml2::XMLElement* effect = outputDAE.NewElement("effect");
        effect->SetAttribute("id", std::format("{}-{}-fx", objectName, materialID).c_str());
        effect->SetAttribute("name", std::format("{}-{}", objectName, materialID).c_str());
        tinyxml2::XMLElement* profile_COMMON = outputDAE.NewElement("profile_COMMON");

        if (exportMaterial.realMaterial)
        {
            tinyxml2::XMLElement* newparamSurface = outputDAE.NewElement("newparam");
            newparamSurface->SetAttribute("sid", std::format("{}-{}-diffuse-surface", objectName, exportMaterial.textureID).c_str());
            tinyxml2::XMLElement* surface = outputDAE.NewElement("surface");
            surface->SetAttribute("type", "2D");
            tinyxml2::XMLElement* init_fromDiffuseSurface = outputDAE.NewElement("init_from");
            init_fromDiffuseSurface->SetText(std::format("{}-{}-diffuse-image", objectName, exportMaterial.textureID).c_str());
            surface->LinkEndChild(init_fromDiffuseSurface);
            newparamSurface->LinkEndChild(surface);
            profile_COMMON->LinkEndChild(newparamSurface);

            tinyxml2::XMLElement* newparamSampler = outputDAE.NewElement("newparam");
            newparamSampler->SetAttribute("sid", std::format("{}-{}-diffuse-sampler", objectName, exportMaterial.textureID).c_str());
            tinyxml2::XMLElement* sampler2D = outputDAE.NewElement("sampler2D");
            tinyxml2::XMLElement* samplerSource = outputDAE.NewElement("source");
            samplerSource->SetText(std::format("{}-{}-diffuse-surface", objectName, exportMaterial.textureID).c_str());
            sampler2D->LinkEndChild(samplerSource);
            newparamSampler->LinkEndChild(sampler2D);
            profile_COMMON->LinkEndChild(newparamSampler);
        }

        tinyxml2::XMLElement* technique = outputDAE.NewElement("technique");
        technique->SetAttribute("sid", "standard");
        tinyxml2::XMLElement* phong = outputDAE.NewElement("phong");
        tinyxml2::XMLElement* diffuse = outputDAE.NewElement("diffuse");

        if (exportMaterial.realMaterial)
        {
            tinyxml2::XMLElement* texture = outputDAE.NewElement("texture");
            texture->SetAttribute("texture", std::format("{}-{}-diffuse-sampler", objectName, exportMaterial.textureID).c_str());
            texture->SetAttribute("texcoord", "CHANNEL0");
            diffuse->LinkEndChild(texture);
        }
        else
        {
            std::string coloursString = "";
            coloursString += std::to_string(rgbToLinearRgb(exportMaterial.redVal) / 1.25f) + " ";
            coloursString += std::to_string(rgbToLinearRgb(exportMaterial.greenVal) / 1.25f) + " ";
            coloursString += std::to_string(rgbToLinearRgb(exportMaterial.blueVal) / 1.25f) + " ";
            coloursString += "1";

            tinyxml2::XMLElement* colour = outputDAE.NewElement("color");
            colour->SetAttribute("sid", "diffuse");
            colour->SetText(coloursString.c_str());
            diffuse->LinkEndChild(colour);
        }
        phong->LinkEndChild(diffuse);
        tinyxml2::XMLElement* specular = outputDAE.NewElement("specular");
        tinyxml2::XMLElement* specularColour = outputDAE.NewElement("color");
        specularColour->SetAttribute("sid", "specular");
        specularColour->SetText("0   0   0   0");
        specular->LinkEndChild(specularColour);
        phong->LinkEndChild(specular);
        tinyxml2::XMLElement* transparency = outputDAE.NewElement("transparency");
        tinyxml2::XMLElement* transparencyFloat = outputDAE.NewElement("float");
        transparencyFloat->SetAttribute("sid", "transparency");
        transparencyFloat->SetText("1");
        transparency->LinkEndChild(transparencyFloat);
        phong->LinkEndChild(transparency);
        technique->LinkEndChild(phong);
        profile_COMMON->LinkEndChild(technique);

        effect->LinkEndChild(profile_COMMON);
        library_effects->LinkEndChild(effect);
    }

    return 0;
}

int exportMaterial(tinyxml2::XMLDocument& outputDAE, tinyxml2::XMLElement* material, tinyxml2::XMLElement* library_materials,
    Material exportMaterial, int materialID, std::string objectName)
{
    material->SetAttribute("id", std::format("{}-mat{}", objectName, materialID).c_str());
    material->SetAttribute("name", std::format("{}-mat{}", objectName, materialID).c_str());
    if (exportMaterial.properlyExported)
    {
        tinyxml2::XMLElement* instance_effectMaterial = outputDAE.NewElement("instance_effect");
        instance_effectMaterial->SetAttribute("url", std::format("#{}-{}-fx", objectName, materialID).c_str());
        material->LinkEndChild(instance_effectMaterial);
    }
    library_materials->LinkEndChild(material);

    return 0;
}

int exportGeometry(tinyxml2::XMLDocument& outputDAE, std::vector<PolygonStruct>& polygons, tinyxml2::XMLElement* geometry, tinyxml2::XMLElement* mesh,
    tinyxml2::XMLElement* library_geometries, std::vector<PolygonStruct>& meshPolygons, std::vector<Vertex>& meshVertices, Material exportMaterial, int materialID,
    std::string objectName)
{
    for (int p = 0; p < polygons.size(); p++)
    {
        if (polygons[p].materialID == materialID)
        {
            meshPolygons.push_back(polygons[p]);
            meshVertices.push_back(polygons[p].v1);
            meshVertices.push_back(polygons[p].v2);
            meshVertices.push_back(polygons[p].v3);
        }
    }

    geometry->SetAttribute("id", std::format("meshId{}", materialID).c_str());
    geometry->SetAttribute("name", std::format("meshId{}_name", materialID).c_str());

    exportPositions(outputDAE, mesh, meshPolygons, meshVertices, materialID);

    exportTextures(outputDAE, mesh, meshPolygons, materialID);

    exportColours(outputDAE, mesh, meshPolygons, exportMaterial, materialID);

    exportVertices(outputDAE, mesh, materialID);

    exportPolygons(outputDAE, mesh, meshPolygons.size(), materialID);

    geometry->LinkEndChild(mesh);
    library_geometries->LinkEndChild(geometry);

    return 0;
}

int exportPositions(tinyxml2::XMLDocument& outputDAE, tinyxml2::XMLElement* mesh,
    std::vector<PolygonStruct>& meshPolygons, std::vector<Vertex>& meshVertices, int materialID)
{
    tinyxml2::XMLElement* positionsSource = outputDAE.NewElement("source");
    positionsSource->SetAttribute("id", std::format("meshId{}-positions", materialID).c_str());
    positionsSource->SetAttribute("name", std::format("meshId{}-positions", materialID).c_str());
    tinyxml2::XMLElement* positionsFloat_array = outputDAE.NewElement("float_array");
    positionsFloat_array->SetAttribute("id", std::format("meshId{}-positions-array", materialID).c_str());
    positionsFloat_array->SetAttribute("count", meshPolygons.size() * 9);
    std::string positionsString = " ";
    for (int v = 0; v < meshVertices.size(); v++)
    {
        positionsString += std::format("{} ", divideByAPowerOfTen(meshVertices[v].finalX, 3));
        positionsString += std::format("{} ", divideByAPowerOfTen(meshVertices[v].finalY, 3));
        positionsString += std::format("{} ", divideByAPowerOfTen(meshVertices[v].finalZ, 3));
    }
    positionsFloat_array->SetText(positionsString.c_str());
    tinyxml2::XMLElement* positionsTechnique_common = outputDAE.NewElement("technique_common");
    tinyxml2::XMLElement* positionsAccessor = outputDAE.NewElement("accessor");
    positionsAccessor->SetAttribute("count", meshPolygons.size() * 3);
    positionsAccessor->SetAttribute("offset", 0);
    positionsAccessor->SetAttribute("source", std::format("#meshId{}-positions-array", materialID).c_str());
    positionsAccessor->SetAttribute("stride", 3);
    tinyxml2::XMLElement* paramX = outputDAE.NewElement("param");
    paramX->SetAttribute("name", "X");
    paramX->SetAttribute("type", "float");
    tinyxml2::XMLElement* paramY = outputDAE.NewElement("param");
    paramY->SetAttribute("name", "Y");
    paramY->SetAttribute("type", "float");
    tinyxml2::XMLElement* paramZ = outputDAE.NewElement("param");
    paramZ->SetAttribute("name", "Z");
    paramZ->SetAttribute("type", "float");

    positionsAccessor->LinkEndChild(paramX);
    positionsAccessor->LinkEndChild(paramY);
    positionsAccessor->LinkEndChild(paramZ);
    positionsTechnique_common->LinkEndChild(positionsAccessor);
    positionsSource->LinkEndChild(positionsFloat_array);
    positionsSource->LinkEndChild(positionsTechnique_common);
    mesh->LinkEndChild(positionsSource);

    return 0;
}

int exportTextures(tinyxml2::XMLDocument& outputDAE, tinyxml2::XMLElement* mesh, std::vector<PolygonStruct>& meshPolygons, int materialID)
{
    tinyxml2::XMLElement* texturesSource = outputDAE.NewElement("source");
    texturesSource->SetAttribute("id", std::format("meshId{}-tex", materialID).c_str());
    texturesSource->SetAttribute("name", std::format("meshId{}-tex", materialID).c_str());
    tinyxml2::XMLElement* texturesFloat_array = outputDAE.NewElement("float_array");
    texturesFloat_array->SetAttribute("id", std::format("meshId{}-tex-array", materialID).c_str());
    texturesFloat_array->SetAttribute("count", meshPolygons.size() * 6);
    std::string texturesString = " ";
    for (int p = 0; p < meshPolygons.size(); p++)
    {
        texturesString += std::format("{} ", meshPolygons[p].uv1.u);
        texturesString += std::format("{} ", meshPolygons[p].uv1.v);
        texturesString += std::format("{} ", meshPolygons[p].uv2.u);
        texturesString += std::format("{} ", meshPolygons[p].uv2.v);
        texturesString += std::format("{} ", meshPolygons[p].uv3.u);
        texturesString += std::format("{} ", meshPolygons[p].uv3.v);
    }
    texturesFloat_array->SetText(texturesString.c_str());
    tinyxml2::XMLElement* texturesTechnique_common = outputDAE.NewElement("technique_common");
    tinyxml2::XMLElement* texturesAccessor = outputDAE.NewElement("accessor");
    texturesAccessor->SetAttribute("count", meshPolygons.size() * 3);
    texturesAccessor->SetAttribute("offset", 0);
    texturesAccessor->SetAttribute("source", std::format("#meshId{}-tex-array", materialID).c_str());
    texturesAccessor->SetAttribute("stride", 2);
    tinyxml2::XMLElement* paramS = outputDAE.NewElement("param");
    paramS->SetAttribute("name", "S");
    paramS->SetAttribute("type", "float");
    tinyxml2::XMLElement* paramT = outputDAE.NewElement("param");
    paramT->SetAttribute("name", "T");
    paramT->SetAttribute("type", "float");

    texturesAccessor->LinkEndChild(paramS);
    texturesAccessor->LinkEndChild(paramT);
    texturesTechnique_common->LinkEndChild(texturesAccessor);
    texturesSource->LinkEndChild(texturesFloat_array);
    texturesSource->LinkEndChild(texturesTechnique_common);
    mesh->LinkEndChild(texturesSource);

    return 0;
}

int exportColours(tinyxml2::XMLDocument& outputDAE, tinyxml2::XMLElement* mesh, std::vector<PolygonStruct>& meshPolygons, Material exportMaterial, int materialID)
{
    tinyxml2::XMLElement* coloursSource = outputDAE.NewElement("source");
    coloursSource->SetAttribute("id", std::format("meshId{}-color", materialID).c_str());
    coloursSource->SetAttribute("name", std::format("meshId{}-color", materialID).c_str());
    tinyxml2::XMLElement* coloursFloat_array = outputDAE.NewElement("float_array");
    coloursFloat_array->SetAttribute("id", std::format("meshId{}-colors-array", materialID).c_str());
    coloursFloat_array->SetAttribute("count", meshPolygons.size() * 9);
    std::string coloursString = " ";
    for (int c = 0; c < meshPolygons.size() * 3; c++)
    {
        if (exportMaterial.realMaterial)
        {
            coloursString += std::to_string(rgbToLinearRgb(exportMaterial.redVal) / 1.25f) + " ";
            coloursString += std::to_string(rgbToLinearRgb(exportMaterial.greenVal) / 1.25f) + " ";
            coloursString += std::to_string(rgbToLinearRgb(exportMaterial.blueVal) / 1.25f) + " ";
        }
        else
            coloursString += "0 0 0 ";
    }
    coloursFloat_array->SetText(coloursString.c_str());
    tinyxml2::XMLElement* coloursTechnique_common = outputDAE.NewElement("technique_common");
    tinyxml2::XMLElement* coloursAccessor = outputDAE.NewElement("accessor");
    coloursAccessor->SetAttribute("count", meshPolygons.size() * 3);
    coloursAccessor->SetAttribute("offset", 0);
    coloursAccessor->SetAttribute("source", std::format("#meshId{}-color-array", materialID).c_str());
    coloursAccessor->SetAttribute("stride", 3);
    tinyxml2::XMLElement* paramR = outputDAE.NewElement("param");
    paramR->SetAttribute("name", "R");
    paramR->SetAttribute("type", "float");
    tinyxml2::XMLElement* paramG = outputDAE.NewElement("param");
    paramG->SetAttribute("name", "G");
    paramG->SetAttribute("type", "float");
    tinyxml2::XMLElement* paramB = outputDAE.NewElement("param");
    paramB->SetAttribute("name", "B");
    paramB->SetAttribute("type", "float");

    coloursAccessor->LinkEndChild(paramR);
    coloursAccessor->LinkEndChild(paramG);
    coloursAccessor->LinkEndChild(paramB);
    coloursTechnique_common->LinkEndChild(coloursAccessor);
    coloursSource->LinkEndChild(coloursFloat_array);
    coloursSource->LinkEndChild(coloursTechnique_common);
    mesh->LinkEndChild(coloursSource);

    return 0;
}

int exportVertices(tinyxml2::XMLDocument& outputDAE, tinyxml2::XMLElement* mesh, int materialID)
{
    tinyxml2::XMLElement* xVertices = outputDAE.NewElement("vertices");
    xVertices->SetAttribute("id", std::format("meshId{}-vertices", materialID).c_str());
    tinyxml2::XMLElement* input = outputDAE.NewElement("input");
    input->SetAttribute("semantic", "POSITION");
    input->SetAttribute("source", std::format("#meshId{}-positions", materialID).c_str());
    xVertices->LinkEndChild(input);
    mesh->LinkEndChild(xVertices);

    return 0;
}

int exportPolygons(tinyxml2::XMLDocument& outputDAE, tinyxml2::XMLElement* mesh, unsigned int meshPolygonsSize, int materialID)
{
    tinyxml2::XMLElement* polylist = outputDAE.NewElement("polylist");
    polylist->SetAttribute("count", meshPolygonsSize);
    polylist->SetAttribute("material", "defaultMaterial");
    tinyxml2::XMLElement* inputVertex = outputDAE.NewElement("input");
    inputVertex->SetAttribute("offset", 0);
    inputVertex->SetAttribute("semantic", "VERTEX");
    inputVertex->SetAttribute("source", std::format("#meshId{}-vertices", materialID).c_str());
    tinyxml2::XMLElement* inputTexCoord = outputDAE.NewElement("input");
    inputTexCoord->SetAttribute("offset", 0);
    inputTexCoord->SetAttribute("semantic", "TEXCOORD");
    inputTexCoord->SetAttribute("source", std::format("#meshId{}-tex", materialID).c_str());
    inputTexCoord->SetAttribute("set", 0);
    tinyxml2::XMLElement* inputColour = outputDAE.NewElement("input");
    inputColour->SetAttribute("offset", 0);
    inputColour->SetAttribute("semantic", "COLOR");
    inputColour->SetAttribute("source", std::format("#meshId{}-color", materialID).c_str());
    inputColour->SetAttribute("set", 0);
    tinyxml2::XMLElement* vCount = outputDAE.NewElement("vcount");
    tinyxml2::XMLElement* pPolylist = outputDAE.NewElement("p");
    std::string vCountString = "";
    std::string polyString = "";
    for (unsigned int p = 0; p < meshPolygonsSize; p++)
    {
        vCountString += "3 ";
        polyString += std::format("{} {} {} ", (p * 3), (p * 3) + 1, (p * 3) + 2);
    }
    vCount->SetText(vCountString.c_str());
    pPolylist->SetText(polyString.c_str());

    polylist->LinkEndChild(inputVertex);
    polylist->LinkEndChild(inputTexCoord);
    polylist->LinkEndChild(inputColour);
    polylist->LinkEndChild(vCount);
    polylist->LinkEndChild(pPolylist);
    mesh->LinkEndChild(polylist);

    return 0;
}

int exportVisualScene(tinyxml2::XMLDocument& outputDAE, tinyxml2::XMLElement* nodeModel, int materialID, std::string objectName)
{
    tinyxml2::XMLElement* instance_geometry = outputDAE.NewElement("instance_geometry");
    instance_geometry->SetAttribute("url", std::format("#meshId{}", materialID).c_str());
    tinyxml2::XMLElement* bind_material = outputDAE.NewElement("bind_material");
    tinyxml2::XMLElement* visualSceneTechnique_common = outputDAE.NewElement("technique_common");
    tinyxml2::XMLElement* instance_material = outputDAE.NewElement("instance_material");
    instance_material->SetAttribute("symbol", "defaultMaterial");
    instance_material->SetAttribute("target", std::format("#{}-mat{}", objectName, materialID).c_str());
    tinyxml2::XMLElement* bind_vertex_input = outputDAE.NewElement("bind_vertex_input");
    bind_vertex_input->SetAttribute("semantic", "CHANNEL0");
    bind_vertex_input->SetAttribute("input_semantic", "TEXCOORD");
    bind_vertex_input->SetAttribute("input_set", 0);

    instance_material->LinkEndChild(bind_vertex_input);
    visualSceneTechnique_common->LinkEndChild(instance_material);
    bind_material->LinkEndChild(visualSceneTechnique_common);
    instance_geometry->LinkEndChild(bind_material);
    nodeModel->LinkEndChild(instance_geometry);

    return 0;
}