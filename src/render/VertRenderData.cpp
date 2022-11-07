#include "VertRenderData.h"
#include "AniInfoManager.h"
#include "Transform.h"

namespace R2grap{

VerticesRenderData::VerticesRenderData(const LayersInfo* data){
  auto shape_offset = data->GetShapeTransform()->GetShapeGrapOffset();
  auto shape_groups = data->GetShapeGroup();
  paths_count_ = 0;

  for(auto& group : shape_groups){
    auto paths = group->GetContents()->GetPaths();
    paths_count_ += static_cast<unsigned int>(paths.size());

    auto final_offset = group->GetTransform()->GetPosition() + shape_offset; 

    for(auto& path : paths){
      multi_paths_data_.clear();
      BezierVertData signal_path_data;

      auto bezier_verts = path->GetBezierVertices();
      auto bezier_verts_count = bezier_verts.size();

      for(auto i = 0; i < bezier_verts_count; i++){
        auto tmp_vert = Normalize<glm::vec2>(bezier_verts[i] + glm::vec2(final_offset.x, final_offset.y));
        signal_path_data.verts.emplace_back(tmp_vert.x);
        signal_path_data.verts.emplace_back(tmp_vert.y);
        signal_path_data.verts.emplace_back(0);
      }
      signal_path_data.tri_index = path->GetTriIndexList();
      bezier_vert_data_.emplace_back(signal_path_data);
    }
  }
}

unsigned int VerticesRenderData::GetVertNumByPathInd(unsigned int ind) const { 
  if(multi_paths_data_.size()){
    return static_cast<unsigned int>(multi_paths_data_[ind].size()); 
  }
  else if(bezier_vert_data_.size()){
    return static_cast<unsigned int>(bezier_vert_data_[ind].verts.size());
  }
  return 0;
}

template<typename T>
T& VerticesRenderData::Normalize(const T& pos){
  auto width = AniInfoManager::GetIns().GetWidth();
  auto height = AniInfoManager::GetIns().GetHeight();
  if(typeid(T) == typeid(glm::vec2)){
    return glm::vec2((pos.x - width/2) / width, (pos.y - height/2) / height);
  }
  else
    return glm::vec3((pos.x - width/2) / width, (pos.y - height/2) / height, 0)
}

bool VerticesRenderData::ConverToOpenglVert(unsigned int path_ind, std::vector<float>& verts){
  auto vertices = multi_paths_data_[path_ind];
  verts.resize(12 * vertices.size());
  auto vert_cluster_ind = 0;
  for(auto& el : vertices){
    verts[vert_cluster_ind * 12] = el.start.x;
    verts[vert_cluster_ind * 12 + 1] = el.start.y;
    verts[vert_cluster_ind * 12 + 2] = 0.0f;
    verts[vert_cluster_ind * 12 + 3] = el.out.x;
    verts[vert_cluster_ind * 12 + 4] = el.out.y;
    verts[vert_cluster_ind * 12 + 5] = 0.0f;
    verts[vert_cluster_ind * 12 + 6] = el.in.x;
    verts[vert_cluster_ind * 12 + 7] = el.in.y;
    verts[vert_cluster_ind * 12 + 8] = 0.0f;
    verts[vert_cluster_ind * 12 + 9] = el.end.x;
    verts[vert_cluster_ind * 12 + 10] = el.end.y;
    verts[vert_cluster_ind * 12 + 11] = 0.0f;
    vert_cluster_ind++;
  }
  return true;
}


}