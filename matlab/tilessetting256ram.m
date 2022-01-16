clear
close all
 

red   = [0,  36,  73, 109, 146, 182, 219, 255];
green = [0,  36,  73, 109, 146, 182, 219, 255];
blue  = [0,  73, 146, 255];

pal = zeros(256,3);
i=1;
for g=0:7
    for r=0:7
        for b=0:3
            pal(i,:) = [red(r+1) green(g+1) blue(b+1)]/255;
            i=i+1;
        end
    end
end

%[A,MAP] = imread('test_level.png');                     % 4096x176 pixels x 256 colori
[A,MAP] = imread('test4.bmp');                     % 4096x176 pixels x 256 colori

[A,pal] = imapprox(A,MAP,pal, 'nodither');

S = fix(size(A)/16).*16;
B = A(1:S(1),1:S(2));

H = S(1);
W = S(2);

image(B);
axis equal;
colormap(MAP);

InpTiles = im2col(B,'indexed',[16 16],'distinct');      % input image
 
% if (0)
% 	RGB = ind2rgb(InpTiles',MAP);
% 	t = [RGB(:,:,1) RGB(:,:,2) RGB(:,:,3)];
% 	[IDX, C] = KMEANS(t, 128,'EmptyAction','singleton','Replicates',10);
% 	UniqueTiles = zeros(128,256,3);
% 	UniqueTiles (:,:,1) = C(:,1:256);
% 	UniqueTiles (:,:,2) = C(:,257:512);
% 	UniqueTiles (:,:,3) = C(:,513:768);
% 	UniqueTiles = rgb2ind(UniqueTiles,MAP);
% 	InpMap=IDX;
% else
    UniqueTiles = unique(InpTiles','rows');
    
    fun = @(block_struct) norm(double(block_struct.data));
    C = blockproc(double(UniqueTiles),[1 256],fun);

    [~,i] = sort(C,1); 
    UniqueTiles = UniqueTiles(i,:);
    
    [~,InpMap] = ismember(InpTiles',UniqueTiles,'rows');
% end

Ntiles = (H/16*W/16);

ReducedImage = UniqueTiles(InpMap,:);

A = col2im(ReducedImage',[16 16],[H W],'distinct');

figure
image(A);
axis equal;
colormap(MAP)

UniqueTiles = UniqueTiles';

NumTiles = size(UniqueTiles,2);
fprintf('Number of unique tiles: %d\n',NumTiles);
T = UniqueTiles;

%if (K<=256)
%    T = [UniqueTiles zeros(256,256-K)];
%    T = UniqueTiles;
%else
%    T = UniqueTiles(:,1:256) ;
%end

%TT = [T(1:16:256,:); T(2:16:256,:); T(3:16:256,:); T(4:16:256,:)]


% fun2 = inline('rgb2ind(ind2rgb(x,M),p,s)','x','M','p','s');
% TT = blkproc(T,[256 1],fun2,MAP,pal,'nodither'); 

TT = T;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% this code is only used to display the tile set

B = col2im(TT,[16 16],[16 16*NumTiles],'distinct');

fun = @(block_struct) transpose(double(block_struct.data));
C = blockproc(double(B),[16 16],fun)';

CC = uint8(zeros((fix(size(C,1)/256)+1)*256,16));
CC(1:size(C,1),:) = C;

D = [];
x = 1;
for i=1:size(CC,1)/256
    D = [D CC(x:(x+255),:)];
    x = x+256;
end

figure
image(D)
axis equal;
colormap(pal)
imwrite(D,pal,'tileset.png','png');

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% show the level map and save it by columns

NewMap = InpMap;

X = reshape(NewMap,H/16,W/16);

figure;
image(X)
axis equal;
colormap(gray)

fid = fopen('datamap.bin','wb');
% for i=1:(H/16)
%     fwrite(fid,X(i,:)-1,'uchar');
% end
for j=1:(W/16)                          % map organised by columns
    fwrite(fid,X(:,j)-1,'uchar');
end

fclose(fid);

ReducedImage = T(:,NewMap);

A = col2im(ReducedImage,[16 16],[H W],'distinct');

figure;
image(A);
axis equal;
colormap(MAP);

imwrite(A,MAP,'output.png','png');

fid = fopen('tileset.bin','wb');
for y = 1:NumTiles
    fwrite(fid,uint8(TT(:,y)),'uchar');
end
fclose(fid);

%figure;
%image(kron(1:256,ones(256)))
%colormap(MAP);
%size(unique(C,'rows'))


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% save the tileset in 4 blocks of 16KB (64 tiles)

!del tile?.bin

NumBlocks = ceil(NumTiles/64);
TPAD = zeros(256,NumBlocks * 64);
TPAD(:,1:NumTiles) = TT;
FileName = 'tile0.bin';
for nb = 1:NumBlocks
    fid = fopen(FileName,'wb');
    for y = 1:64
        fwrite(fid,uint8(TPAD(:,y+(nb-1)*64)),'uchar');
    end
    fclose(fid);
    FileName(5) = FileName(5) + 1;
end


!copy tile?.bin ..\..\dsk\dska
!copy datamap.bin ..\..\dsk\dska
