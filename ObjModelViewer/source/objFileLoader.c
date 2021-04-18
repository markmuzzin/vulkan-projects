#include "objFileLoader.h"

void addFloatData(float **buffer, float *data, uint32_t *numOfData, uint32_t count)
{
    uint32_t i;
    for(i=0;i<count;i++)
    {
        *buffer = (float *)realloc(*buffer, ((*numOfData)+1) * sizeof(float));
        *(*buffer+(*numOfData)) = *(data+i);
        (*numOfData)++;
    }
}


void addIntegerData(uint32_t **buffer, uint32_t *data, uint32_t *numOfData, uint32_t count)
{
    uint32_t i;

    for(i=0;i<count;i++)
    {
        *buffer = (uint32_t *)realloc(*buffer, ((*numOfData)+1) * sizeof(uint32_t));
        *(*buffer+(*numOfData)) = *(data+i);
        (*numOfData)++;
    }
}


VkBool32 checkPrefix(char *line, char *expPrefix)
{
    return (strncmp(line, expPrefix, strlen(expPrefix)) == 0) ? VK_TRUE : VK_FALSE;
}


void setMaterialDefaults(material_t *material)
{
    material->mp.Ns         = 100.0f;

    material->mp.Ka.x       = 0.0f;
    material->mp.Ka.y       = 0.0f;
    material->mp.Ka.z       = 0.0f;

    material->mp.Kd.x       = 1.0;
    material->mp.Kd.y       = 1.0;
    material->mp.Kd.z       = 1.0;

    material->mp.Ks.x       = 0.01f;
    material->mp.Ks.y       = 0.01f;
    material->mp.Ks.z       = 0.01f;

    material->mp.Ni         = 1.0f;
    material->mp.d          = 1.0f;
    material->mp.illum      = 1.0f;

    material->mp.imageIndex = 0;
    material->mp.shininess  = 32;
}


char *getPath(char *string)
{
    char *path = NULL;
    uint64_t position = strlen(string);

    while(position != 0 && string[--position] != '\\');

    path = (char *)malloc(position+2);
    memset(path, 0, position+2);
    strncpy_s(path, position+2, string, position+1);

    /* Switch to forward slash */
    path[position] = '/';

    return path;
}


VkBool32 loadMtlFile(model_t *model, material_t *materials, char *mtlFilename)
{
    char line[STRLEN];
    char stringName[STRLEN];
    char keyword[STRLEN];
    FILE *pFile;
    uint32_t i;
    VkBool32 endOfEntry = VK_FALSE;
    errno_t err;

    err = fopen_s(&pFile, mtlFilename, "rb");
    if (err != 0)
    {
       printf("Error opening MTL file\n");
       return VK_FALSE;
    }

    /* Get the line entry */
    while(fgets(line, STRLEN, pFile) != NULL)
    {
        if ( checkPrefix(line, "newmtl ") )
        {
            endOfEntry = VK_FALSE;
            sscanf_s(line, "%s %s", keyword, STRLEN, stringName, STRLEN);

            for(i=0;i<model->materialCount && !endOfEntry;i++)
            {
                /* Check if newmtl matches one of the names in the material list */
                if( 0 == strcmp(materials[i].name, stringName) )
                {
                    /* Set the default values in case the material file does not specify them */
                    setMaterialDefaults(&materials[i]);

                    /* If so, cycle through each entry until a blank line is found */
                    while(!endOfEntry)
                    {
                        memset(line, 0, STRLEN);
                        fgets(line, STRLEN, pFile);

                        if( checkPrefix(line, "Ns ") )
                        {
                            sscanf_s(line, "%s %f", keyword, STRLEN, &materials[i].mp.Ns);
                        }
                        else if( checkPrefix(line, "Ka ") )
                        {
                            sscanf_s(line, "%s %f %f %f", keyword, STRLEN, &materials[i].mp.Ka.x, &materials[i].mp.Ka.y, &materials[i].mp.Ka.z);
                        }
                        else if( checkPrefix(line, "Kd ") )
                        {
                            sscanf_s(line, "%s %f %f %f", keyword, STRLEN, &materials[i].mp.Kd.x, &materials[i].mp.Kd.y, &materials[i].mp.Kd.z);
                        }
                        else if( checkPrefix(line, "Ks ") )
                        {
                            sscanf_s(line, "%s %f %f %f", keyword, STRLEN, &materials[i].mp.Ks.x, &materials[i].mp.Ks.y, &materials[i].mp.Ks.z);
                        }
                        else if( checkPrefix(line, "Ni ") )
                        {
                            sscanf_s(line, "%s %f", keyword, STRLEN, &materials[i].mp.Ni);
                        }
                        else if( checkPrefix(line, "d ") )
                        {
                            sscanf_s(line, "%s %f", keyword, STRLEN, &materials[i].mp.d);
                        }
                        else if( checkPrefix(line, "illum ") )
                        {
                            sscanf_s(line, "%s %f", keyword, STRLEN, &materials[i].mp.illum);
                        }
                        else if( checkPrefix(line, "map_Kd ") )
                        {
                            sscanf_s(line, "%s %s", keyword, STRLEN, stringName, STRLEN);
                            materials[i].fileName = (char *)malloc(strlen(stringName)+1);
                            memset(materials[i].fileName, 0, strlen(stringName) + 1);
                            strcpy_s(materials[i].fileName, strlen(stringName) + 1, stringName);
                        }
                        else
                        {
                            /* Check for blank line */
                            if ( strlen(line) <= 2 )
                            {
                                  endOfEntry = VK_TRUE;
                            }
                        }
                    }
                }
            }
        }
    }

    fclose(pFile);

    return VK_TRUE;
}


VkBool32 loadObjFile(model_t *model, material_t *materials, char *objFilename)
{
    char prefix[STRLEN];
    char line[STRLEN];
    char stringName[STRLEN];
    char keyword[STRLEN];
    float value[3];
    int32_t indices[9];
    FILE *pFile = NULL;
    VkBool32 haveTexture = VK_FALSE;
    uint32_t i;
    errno_t err;

    /* Open object file */
    err = fopen_s(&pFile, objFilename, "r");
    if (err != 0)
    {
       printf("Error opening OBJ file\n");
       return VK_FALSE;
    }

    /* Get the line entry */
    while(fgets(line, STRLEN, pFile) != NULL)
    {
        if( checkPrefix(line, "v ") )
        {
            sscanf_s(line, "%s %f %f %f", prefix, STRLEN, &value[0], &value[1], &value[2]);
            addFloatData(&model->v, value, &model->numOfVertices, 3);
        }
        else if( checkPrefix(line, "vt ") )
        {
            sscanf_s(line, "%s %f %f", prefix, STRLEN, &value[0], &value[1]);
            addFloatData(&model->vt, value, &model->numOfTexCoords, 2);
        }
        else if( checkPrefix(line, "vn ") )
        {
            sscanf_s(line, "%s %f %f %f", prefix, STRLEN, &value[0], &value[1], &value[2]);
            addFloatData(&model->vn, value, &model->numOfNormals, 3);
        }
        else if( checkPrefix(line, "f ") )
        {
            if(model->numOfNormals == 0)
            {
                sscanf_s(line, "%s %d/%d %d/%d %d/%d", prefix, STRLEN,
                                &indices[0], &indices[1],
                                &indices[2], &indices[3],
                                &indices[4], &indices[5]);

                addIntegerData(&model->f, (uint32_t*)indices, &model->numOfFaces, 6);
            }
            else
            {
                sscanf_s(line, "%s %d/%d/%d %d/%d/%d %d/%d/%d", prefix, STRLEN,
                                &indices[0], &indices[1], &indices[2],
                                &indices[3], &indices[4], &indices[5],
                                &indices[6], &indices[7], &indices[8]);
                addIntegerData(&model->f, (uint32_t*)indices, &model->numOfFaces, 9);
            }
        }
        else if( checkPrefix(line, "usemtl ") )
        {
            /* Reset flag */
            haveTexture = VK_FALSE;

            sscanf_s(line, "%s %s", keyword, STRLEN, stringName, STRLEN);

            /* Check if we have this texture */
            for(i=0;i<model->materialCount;i++)
            {
                if(0 == strcmp(materials[i].name, stringName))
                {
                    /* We have this material already */
                    haveTexture = VK_TRUE;

                    /* Mark the start face that uses this texture */
                    model->materialChange[model->materialChangeCount].startFace = model->numOfFaces;
                    model->materialChange[model->materialChangeCount].startFace /= (ELEMENTS_PER_FACE*ELEMENTS_PER_VERTEX);
                    model->materialChange[model->materialChangeCount].material = &materials[i];
                    model->materialChangeCount++;
                    break;
                }
            }

            /* If we dont have this texture name stored yet */
            if ( VK_FALSE == haveTexture )
            {
                /* Add material name */
                materials[model->materialCount].name = (char *)malloc(strlen(stringName)+1);
                memset(materials[model->materialCount].name, 0, strlen(stringName)+1);
                strcpy_s(materials[model->materialCount].name, strlen(stringName)+1, stringName);

                /* Mark the start face that uses this texture */
                model->materialChange[model->materialChangeCount].startFace = model->numOfFaces;
                model->materialChange[model->materialChangeCount].startFace /= (ELEMENTS_PER_FACE*ELEMENTS_PER_VERTEX);
                model->materialChange[model->materialChangeCount].material = &materials[model->materialCount];
                model->materialChangeCount++;

                /* Increment material count */
                model->materialCount++;
            }
        }
        else if( checkPrefix(line, "mtllib ") )
        {
            sscanf_s(line, "%s %s", keyword, STRLEN, stringName, STRLEN);

            /* Add material filename */
            model->materialLibFilename = (char *)malloc(strlen(stringName)+1);
            memset(model->materialLibFilename, 0, strlen(stringName)+1);
            strcpy_s(model->materialLibFilename, strlen(stringName)+1, stringName);
        }

        else if( checkPrefix(line, "lightpos ") )
        {
            sscanf(line, "%s %f %f %f", prefix, &model->sp.lightPosition.x, &model->sp.lightPosition.y, &model->sp.lightPosition.z);
        }

        else if( checkPrefix(line, "campos ") )
        {
            sscanf(line, "%s %f %f %f", prefix, &model->cameraPosition.x, &model->cameraPosition.y, &model->cameraPosition.z);
        }
        else if( checkPrefix(line, "camfront ") )
        {
            sscanf(line, "%s %f %f %f", prefix, &model->cameraFront.x, &model->cameraFront.y, &model->cameraFront.z);
        }
        else if( checkPrefix(line, "camup ") )
        {
            sscanf(line, "%s %f %f %f", prefix, &model->cameraUp.x, &model->cameraUp.y, &model->cameraUp.z);
        }
        else
        {
            /* Do Nothing */
        }
    }

    model->numOfVertices /= ELEMENTS_PER_VERTEX;
    model->numOfTexCoords /= ELEMENTS_PER_TEXCOORDS;
    model->numOfNormals /= ELEMENTS_PER_VERTEX;

    /* Check if we have normals in the file, calculate the face count accordingly */
    model->numOfFaces /= (model->numOfNormals) ? (ELEMENTS_PER_FACE*3) : (ELEMENTS_PER_FACE*2);

    fclose (pFile);

    return VK_TRUE;
}


void cleanUp(model_t *model)
{
    if( model->vertArray != NULL )
    {
        free(model->vertArray);
    }

    if( model->texArray != NULL )
    {
        free(model->texArray);
    }

    if( model->normArray != NULL )
    {
        free(model->normArray);
    }
}


VkBool32 loadModel(model_t *model, material_t *materials, char *objFileName)
{
    uint32_t i;
    char *mtlFile;
    char *path;

    mtlFile = getPath(objFileName);
    path = getPath(objFileName);

    /* Load and parse obj file */
    printf("Loading object file: %s...", objFileName);

    if ( VK_FALSE == loadObjFile(model, materials, objFileName) )
    {
        cleanUp(model);
        return VK_FALSE;
    }
    printf("done\n");

    /* Construct material filename */
    mtlFile = (char*)realloc(mtlFile, (strlen(mtlFile) + strlen(model->materialLibFilename))+1);
    strncat_s(mtlFile, strlen(mtlFile) + strlen(model->materialLibFilename)+1, model->materialLibFilename, strlen(model->materialLibFilename)+1);

    /* Load and parse material file */
    printf("Loading mtl file: %s...", mtlFile);
    if ( VK_FALSE == loadMtlFile(model, materials, mtlFile) )
    {
        return VK_FALSE;
    }
    printf("done\n");

    printf("\tvertices:\t\t%d\n", model->numOfVertices);
    printf("\ttex coords:\t\t%d\n", model->numOfTexCoords);
    printf("\tnormals:\t\t%d\n", model->numOfNormals);
    printf("\tfaces:\t\t\t%d\n", model->numOfFaces);
    printf("\ttotal vertices drawn:\t%d\n", model->numOfFaces*ELEMENTS_PER_FACE);
    printf("\tmaterials:\t\t%d\n", model->materialCount);

    for(i=0; i<model->materialCount; i++)
    {
        printf("\t%3d: %-30s\tfilename: %-30s\n", i, materials[i].name, materials[i].fileName);
    }

    free(mtlFile);
    free(path);

    return VK_TRUE;
}


void prepareObjectArrays(model_t *model)
{
    uint32_t i, j;
    uint32_t v[3];
    uint32_t vt[3];
    uint32_t vn[3];
    uint32_t vc = 0;

    /* Assign pointers to input values */
    uint32_t *faces = (uint32_t *)model->f;

    uint32_t totalSize = (sizeof(float)*model->numOfFaces*(ELEMENTS_PER_FACE*(ELEMENTS_PER_VERTEX+1))) * 2 \
                          + sizeof(float)*model->numOfFaces*(ELEMENTS_PER_FACE*(ELEMENTS_PER_VERTEX+1));

    /* Create vertex and texture buffers */
    model->vertArray = (float *)malloc(totalSize);

    uint32_t stride = (model->numOfNormals == 0) ? (ELEMENTS_PER_FACE*ELEMENTS_PER_TEXCOORDS) : (ELEMENTS_PER_FACE*ELEMENTS_PER_VERTEX);
    uint32_t offset = (model->numOfNormals == 0) ? 0 : 1;

    for(i=0;i<model->numOfFaces;i++)
    {
        /* Get offsets from face inputs */
        v[0]  = faces[(stride*i)+0]-1;
        vt[0] = faces[(stride*i)+1]-1;
        v[1]  = faces[(stride*i)+2+offset]-1;
        vt[1] = faces[(stride*i)+3+offset]-1;
        v[2]  = faces[(stride*i)+4+(offset*2)]-1;
        vt[2] = faces[(stride*i)+5+(offset*2)]-1;

        /* Load vertices */
        for(j=0; j<3; j++)
        {
            model->vertArray[vc++] = *(model->v + (v[j]*ELEMENTS_PER_VERTEX + 0));
            model->vertArray[vc++] = *(model->v + (v[j]*ELEMENTS_PER_VERTEX + 1));
            model->vertArray[vc++] = *(model->v + (v[j]*ELEMENTS_PER_VERTEX + 2));
            model->vertArray[vc++] = 1.0f;

            /* If this file contains normals */
            if(model->numOfNormals != 0)
            {
                vn[0] = faces[(stride*i)+2]-1;
                vn[1] = faces[(stride*i)+5]-1;
                vn[2] = faces[(stride*i)+8]-1;

                model->vertArray[vc++] = *(model->vn + (vn[j]*ELEMENTS_PER_VERTEX + 0));
                model->vertArray[vc++] = *(model->vn + (vn[j]*ELEMENTS_PER_VERTEX + 1));
                model->vertArray[vc++] = *(model->vn + (vn[j]*ELEMENTS_PER_VERTEX + 2));
                model->vertArray[vc++] = 0.0f;
            }

            model->vertArray[vc++] = *(model->vt + (vt[j]*ELEMENTS_PER_TEXCOORDS + 0));
            model->vertArray[vc++] = *(model->vt + (vt[j]*ELEMENTS_PER_TEXCOORDS + 1));
        }
    }
}
