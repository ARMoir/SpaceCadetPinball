"""TRamp/TOneway source-data reduction. All geometry work is offline (F-002)."""
import math
import struct


def collision_mask(dat, group):
    mask = 0
    raw = dat.field(group, 10) or b''
    words = struct.unpack('<'+'h'*(len(raw)//2), raw)
    for i in range(0, len(words)-1, 2):
        if words[i] == 602:
            if not 0 <= words[i+1] < 15:
                raise ValueError('Unsupported collision group')
            mask |= 1 << words[i+1]
    return mask or 1


def ramp_path(dat, group, project):
    values = dat.attr(group, 1300)
    count = int(values[0])
    if count < 1 or count > 22 or len(values) != 1+count*13:
        raise ValueError('Invalid ramp plane array')
    planes = [values[1+i*13:1+(i+1)*13] for i in range(count)]
    triangles = [[tuple(p[j:j+2]) for j in (3,5,7)] for p in planes]
    walls = [dat.attr(group, key) for key in (1301,1302)]
    # maths::find_closest_edge: ordered endpoint Euclidean distances.
    def closest(w):
        return min(((math.dist(w[3:5],a)+math.dist(w[5:7],b),i,a,b)
                    for i,t in enumerate(triangles)
                    for a,b in zip(t,t[1:]+t[:1])), key=lambda x:x[0])[1:]
    ends = [closest(w) for w in walls]
    # Start at ground admission; the flat return has only wall2 enabled.
    first = 0 if walls[0][1] and int(walls[0][0]) == 0 else 1
    start, finish = ends[first], ends[1-first]
    queue = [(start[0], [start[0]], [])]
    route = None
    while queue:
        index, visited, portals = queue.pop(0)
        if index == finish[0]:
            route = portals
            break
        for j,t in enumerate(triangles):
            shared = [p for p in triangles[index] if p in t]
            if j not in visited and len(shared) == 2:
                queue.append((j, visited+[j], portals+[shared]))
    if route is None:
        raise ValueError('Disconnected ramp triangles')
    mid = lambda a,b: tuple((x+y)/2 for x,y in zip(a,b))
    world = [mid(start[1],start[2])] + [mid(*p) for p in route] + [mid(finish[1],finish[2])]
    points = [tuple(round(v*64) for v in project(p)) for p in world]
    segments, total = [], 0
    for a,b in zip(points,points[1:]):
        dx,dy = b[0]-a[0],b[1]-a[1]
        length = round(math.hypot(dx,dy))
        if not length:
            continue
        segments.append([*a,*b,round(dx*256/length),round(dy*256/length),length,total])
        total += length
    def height(which, pt):
        p=planes[which]
        return p[0]*pt[0]+p[1]*pt[1]+p[2]
    rise = height(finish[0],world[-1])-height(start[0],world[0])
    return {'group':group,'name':dat.name(group),'mask':collision_mask(dat,group),
            'plane_count':count,'planes':planes,'walls':walls,
            'control_line':dat.attr(group,1303),'height_flag':dat.attr(group,1305),
            'entry_attribute':1301+first,'path_q6':points,'segments':segments,
            'length_q6':total,'rise':rise,'assist':int(rise>0.1),
            'damping':dat.attr(group,701,(0.2,))[0]}


def directional_line(a,b,offset,project):
    dx,dy=b[0]-a[0],b[1]-a[1]
    length=math.hypot(dx,dy)
    if not length: raise ValueError('Degenerate directional line')
    normal=(dy/length,-dx/length)  # maths::line_init
    a=tuple(a[i]+offset*normal[i] for i in range(2))
    b=tuple(b[i]+offset*normal[i] for i in range(2))
    pa,pb=project(a),project(b)
    mid=tuple((a[i]+b[i])/2 for i in range(2))
    pm,pn=project(mid),project(tuple(mid[i]+normal[i] for i in range(2)))
    dx,dy=pb[0]-pa[0],pb[1]-pa[1]
    length=math.hypot(dx,dy)
    nx,ny=dy/length,-dx/length
    if nx*(pn[0]-pm[0])+ny*(pn[1]-pm[1])<0: nx,ny=-nx,-ny
    return [*(round(v*64) for p in (pa,pb) for v in p),round(nx*256),round(ny*256),round(length*64)]


def components(dat, objects, project):
    ramps, lines, gates = [], [], []
    radius=dat.attr(dat.named('ball'),500)[0]
    for kind,group in objects:
        if kind == 1021:
            ramps.append(ramp_path(dat,group,project))
        elif kind in (1016,1013):
            wall=dat.attr(group,600)
            if not wall or int(wall[0]) != 2: raise ValueError('Expected component line')
            a,b=wall[1:3],wall[3:5]
            item={'group':group,'name':dat.name(group),'mask':collision_mask(dat,group),'world':wall}
            if kind == 1016:
                item['block']=directional_line(a,b,radius,project)
                item['pass']=directional_line(b,a,-radius*0.8,project)
                lines.append(item)
            else:
                item['projected']=[project(a),project(b)]
                gates.append(item)
    if len(ramps)>2 or len(lines)>12: raise ValueError('Component capacity exceeded')
    return {'ramps':ramps,'oneways':lines,'gates':gates,'radius':radius}


def pack_components(data):
    body=b'CMP2'+struct.pack('<2h',len(data['ramps']),len(data['oneways'])*2)
    for ramp in data['ramps']:
        segments=ramp['segments']
        body+=struct.pack('<4h',ramp['group'],len(segments),ramp['length_q6'],ramp['assist'])
        for s in segments: body+=struct.pack('<8h',*s)
    for item in data['oneways']:
        for name,solid in (('block',1),('pass',0)):
            body+=struct.pack('<10h',item['group'],item['mask'],solid,*item[name])
    # TGate 501/502, in source control gate1/left and gate2/right order.
    gates=sorted(data['gates'],key=lambda g:g['name'])
    if [g['name'] for g in gates]!=['v_gate1','v_gate2']:
        raise ValueError('Expected both source kicker gates')
    for gate in gates:
        if gate['mask']!=1: raise ValueError('Expected ground kicker gate')
        a,b=gate['projected']
        dx,dy=b[0]-a[0],b[1]-a[1]
        length=math.hypot(dx,dy)
        body+=struct.pack('<9h',*(round(v) for p in (a,b) for v in p),
                          round(dx*256/length),round(dy*256/length),
                          round(length*64),0,0)
    return body

def validate_extension(body):
    """Validate extension framing for asset tools that do not execute physics."""
    if not body: return
    if body[:4] not in (b'CMP1',b'CMP2') or len(body)<8: raise ValueError('Bad CMP1 header')
    ramps,lines=struct.unpack_from('<2h',body,4)
    if not 0<=ramps<=2 or not 0<=lines<=24: raise ValueError('Component capacity')
    pos=8
    for _ in range(ramps):
        if pos+8>len(body): raise ValueError('Truncated ramp')
        group,count,length,assist=struct.unpack_from('<4h',body,pos)
        if not 1<=count<=24 or not 1<=length<=16000 or assist not in (0,1):
            raise ValueError('Invalid ramp')
        pos+=8+count*16
    pos+=lines*20
    if body[:4]==b'CMP2': pos+=36
    if pos!=len(body): raise ValueError('Truncated or trailing CMP1')
